﻿using NetMud.Communication;
using NetMud.DataAccess;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Caching;
using System.Threading;
using System.Threading.Tasks;

namespace NetMud.CentralControl
{
    /// <summary>
    /// Central handler for all things async and recursive
    /// </summary>
    public static class Processor
    {
        private static ObjectCache globalCache = MemoryCache.Default;
        private static CacheItemPolicy globalPolicy = new CacheItemPolicy();
        private static string cacheKeyFormat = "AsyncCancellationToken.{0}";

        /// <summary>
        /// Starts a brand new Loop
        /// </summary>
        /// <param name="designator">what the loop will be called</param>
        /// <param name="rampupDelay">In # of Seconds - how long to wait before performing the first operation on start</param>
        /// <param name="cooldownDelay">In # of Seconds - how long to wait before either shutting the loop down or recurring it</param>
        /// <param name="maxDuration">In # of Seconds - kill this loop after this many seconds</param>
        /// <param name="process">What delegate process to call for the loop</param>
        /// <param name="tailProcess">What delegate process to call when this loop terminates</param>
        public static void StartNewLoop(string designator, int rampupDelay, int cooldownDelay, int maxDuration
            , Func<bool> workProcess, Func<bool> successTailProcess = null, Func<bool> failedTailProcess = null)
        {
            var cancelTokenSource = new CancellationTokenSource();

            if(maxDuration > 0)
                cancelTokenSource.CancelAfter(maxDuration * 1000); //seconds * 1000 for miliseconds

            StoreCancellationToken(designator, cancelTokenSource);

            Func<bool> loopedProcess = () =>
                {
                    StartLoop(workProcess, rampupDelay);
                    return true;
                };

            var newLoop = new Task<bool>(loopedProcess, cancelTokenSource.Token, TaskCreationOptions.LongRunning);

            //Setup callbacks to termination tail chained functions and make sure we remove the token.
            if (successTailProcess != null && failedTailProcess != null)
            {
                newLoop.ContinueWith(async (previousTask) =>
                    {
                        await Task.Delay(cooldownDelay * 1000);

                        if (!previousTask.IsFaulted)
                            successTailProcess.Invoke();
                        else
                            failedTailProcess.Invoke();

                        RemoveCancellationToken(designator);
                    }
                );
            }
            else
            {
                newLoop.ContinueWith(async (previousTask) =>
                    {
                        await Task.Delay(cooldownDelay * 1000);

                        RemoveCancellationToken(designator);
                    }
                );
            }

            newLoop.Start();
        }

        /// <summary>
        /// Starts an endless loop
        /// </summary>
        /// <param name="worker">The action to perform every cycle</param>
        /// <param name="startDelay">The amount of seconds to delay before running the worker</param>
        private static async void StartLoop(Func<bool> worker, int startDelay)
        {
            await Task.Delay(startDelay * 1000);
            worker.Invoke();
            StartLoop(worker, startDelay);
        }

        /// <summary>
        /// Tries to shutdown an existing loop
        /// </summary>
        /// <param name="designator">what the loop is called</param>
        /// <param name="shutdownDelay">In # of Seconds - how long to wait before cancelling it</param>
        /// <param name="shutdownAnnouncementFrequency">In # of Seconds - how often to announce the shutdown, <= 0 is only one announcement</param>
        /// <param name="shutdownAnnouncement">What to announce (string.format format) before shutting down, empty string = no announcements, ex "World shutting down in {0} seconds."</param>
        public static void ShutdownLoop(string designator, int shutdownDelay, string shutdownAnnouncement, int shutdownAnnouncementFrequency = -1)
        {
            var cancelToken = GetCancellationToken(designator);

            if (cancelToken != null && cancelToken.Token.CanBeCanceled)
                cancelToken.CancelAfter(shutdownDelay * 1000);

            if (!string.IsNullOrWhiteSpace(shutdownAnnouncement))
                SystemComm.BroadcastToAll(String.Format(shutdownAnnouncement, shutdownDelay));

            if (shutdownAnnouncementFrequency > 0)
            {
                Task.Run(() => RunAnnouncements(shutdownDelay, shutdownAnnouncement, shutdownAnnouncementFrequency));
            }
        }

        /// <summary>
        /// Runs looped formatted messages until the timer runs out
        /// </summary>
        /// <param name="shutdownDelay">Total amount of message time</param>
        /// <param name="shutdownAnnouncement">What to announce</param>
        /// <param name="shutdownAnnouncementFrequency">How often to announce it</param>
        private static async void RunAnnouncements(int shutdownDelay, string shutdownAnnouncement, int shutdownAnnouncementFrequency)
        {
            int secondsLeftBeforeShutdown = shutdownDelay;
            while (secondsLeftBeforeShutdown > 0)
            {
                SystemComm.BroadcastToAll(String.Format(shutdownAnnouncement, secondsLeftBeforeShutdown));
                await Task.Delay(shutdownAnnouncementFrequency * 1000);
                secondsLeftBeforeShutdown -= shutdownAnnouncementFrequency;
            }
        }

        /// <summary>
        /// Get all the living task cancellation tokens
        /// </summary>
        /// <returns>All the tokens in the live cache</returns>
        public static Dictionary<string, CancellationTokenSource> GetAllLiveTaskStatusTokens()
        {
            var returnDict = new Dictionary<string, CancellationTokenSource>();
            foreach (var kvp in globalCache.Where(kvp => kvp.Value.GetType() == typeof(CancellationTokenSource)))
                returnDict.Add(kvp.Key.Replace("AsyncCancellationToken.", String.Empty), (CancellationTokenSource)kvp.Value);

            return returnDict;
        }

        /// <summary>
        /// Cancel ALL the current loops
        /// </summary>
        /// <param name="shutdownDelay">In # of Seconds - how long to wait before cancelling it</param>
        /// <param name="shutdownAnnouncementFrequency">In # of Seconds - how often to announce the shutdown, < = 0 is only one announcement</param>
        /// <param name="shutdownAnnouncement">What to announce (string.format format) before shutting down, empty string = no announcements</param>
        public static void ShutdownAll(int shutdownDelay, string shutdownAnnouncement, int shutdownAnnouncementFrequency = -1)
        {
            IEnumerable<CancellationTokenSource> tokens 
                = globalCache.Where(kvp => kvp.Value.GetType() == typeof(CancellationTokenSource)).Select(kvp => (CancellationTokenSource)kvp.Value);

            foreach (var token in tokens.Where(tk => !tk.IsCancellationRequested && tk.Token.CanBeCanceled))
            {
                if (token != null)
                    token.CancelAfter(shutdownDelay * 1000);
            }

            if (!string.IsNullOrWhiteSpace(shutdownAnnouncement))
                SystemComm.BroadcastToAll(String.Format(shutdownAnnouncement, shutdownDelay));

            if (shutdownAnnouncementFrequency > 0)
            {
                Task.Run(() => RunAnnouncements(shutdownDelay, shutdownAnnouncement, shutdownAnnouncementFrequency));
            }
        }

        /// <summary>
        /// Store the cancel token in the cache
        /// </summary>
        /// <param name="designator">what the loop is called</param>
        /// <param name="token">the token to store</param>
        private static void StoreCancellationToken(string designator, CancellationTokenSource token)
        {
            globalCache.AddOrGetExisting(String.Format(cacheKeyFormat, designator), token, globalPolicy);
        }

        /// <summary>
        /// Gets the token from the cache
        /// </summary>
        /// <param name="designator">what the loop is called</param>
        /// <returns>the token</returns>
        private static CancellationTokenSource GetCancellationToken(string designator)
        {
            try
            {
                return (CancellationTokenSource)globalCache[String.Format(cacheKeyFormat, designator)];
            }
            catch (Exception ex)
            {
                LoggingUtility.LogError(ex);
            }

            return null;
        }

        /// <summary>
        /// Removes a token from the cache
        /// </summary>
        /// <param name="designator">The token's designator</param>
        private static void RemoveCancellationToken(string designator)
        {
            globalCache.Remove(String.Format(cacheKeyFormat, designator));
        }
    }
}
