using NetMud.Communication;
using NetMud.DataAccess;
using System;
using System.Linq;
using System.Runtime.Caching;
using WebSocketSharp;
using WebSocketSharp.Server;

namespace NetMud.Websock
{
    /// <summary>
    /// A websocket client server
    /// </summary>
    public class Server : Channel, IServer
    {
        /// <summary>
        /// The place everything gets stored
        /// </summary>
        private ObjectCache globalCache = MemoryCache.Default;

        /// <summary>
        /// The general storage policy
        /// </summary>
        private CacheItemPolicy globalPolicy = new CacheItemPolicy();

        /// <summary>
        /// Broadcast a message to all ports
        /// </summary>
        /// <param name="message">the message to send</param>
        /// <returns>Whether or not the service sent it</returns>
        public bool Broadcast(string message)
        {
            var services = globalCache.Where(keyValuePair =>
                    keyValuePair.Value.GetType() == typeof(WebSocketServiceManager)).Select(kvp => (WebSocketServiceManager)kvp.Value);

            foreach (var service in services)
                service.Broadcast(message);

            return true;
        }

        /// <summary>
        /// Broadcast a message to one port
        /// </summary>
        /// <param name="message">the message to send</param>
        /// <param name="portNumber">the port to send to</param>
        /// <returns>Whether or not the service sent it</returns>
        public bool Broadcast(string message, int portNumber)
        {
            var service = GetActiveService<WebSocketServiceManager>(portNumber);

            if (service == null)
                return false;

            service.Broadcast(message);

            return true;
        }

        /// <summary>
        /// Gets an active listener service
        /// </summary>
        /// <param name="portNumber">the port it is listening on</param>
        /// <returns>the service</returns>
        public T GetActiveService<T>(int portNumber)
        {
            try
            {
                return (T)globalCache[String.Format(cacheKeyFormat, portNumber)];
            }
            catch (Exception ex)
            {
                LoggingUtility.LogError(ex);
            }

            return default(T);
        }

        /// <summary>
        /// Start a new websocket server
        /// </summary>
        /// <param name="portNumber">the port to listen on</param>
        /// <returns>The service manage (which you need to hold on to cause there isn't a way to get this back later)</returns>
        public void Launch(int portNumber)
        {
            var service = StartServer(String.Empty, portNumber);

            globalCache.AddOrGetExisting(String.Format(cacheKeyFormat, portNumber), service, globalPolicy);
        }

        /// <summary>
        /// Shuts down a websocket server
        /// </summary>
        /// <param name="portNumber">The port to shutdown, or -1 for all</param>
        public void Shutdown(int portNumber = -1)
        {
            var service = GetActiveService<WebSocketServiceManager>(portNumber);

            Broadcast("Shutting down.");

            WebSocketServiceHost host;
            service.TryGetServiceHost("/", out host);

            foreach (var id in host.Sessions.ActiveIDs)
                host.Sessions.CloseSession(id);
        }

        private WebSocketServiceManager StartServer(string domain, int portNumber)
        {
            try
            {
                var wssv = new WebSocketServer(portNumber);

#if DEBUG
                // To change the logging level.
                wssv.Log.Level = LogLevel.Trace;

                // To change the wait time for the response to the WebSocket Ping or Close.
                wssv.WaitTime = TimeSpan.FromSeconds(2);
#endif

                wssv.AddWebSocketService<Descriptor>("/");

                wssv.Start();

                return wssv.WebSocketServices;
            }
            catch (Exception ex)
            {
                LoggingUtility.LogError(ex);
            }

            return null;
        }
    }
}
