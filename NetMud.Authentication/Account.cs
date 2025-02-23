﻿
using NetMud.DataAccess;
using NetMud.DataStructure.Base.EntityBackingData;
using NetMud.DataStructure.Base.System;
using NetMud.Utility;
using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.ComponentModel.DataAnnotations.Schema;
using System.Data;
using System.Linq;

namespace NetMud.Authentication
{
    /// <summary>
    /// User account
    /// </summary>
    public class Account : IAccount
    {
        /// <summary>
        /// Unique identifier AND forum/chat handle for player's user account
        /// </summary>
        [Key]
        public string GlobalIdentityHandle { get; set; }

        /// <summary>
        /// New up a blank account
        /// </summary>
        public Account()
        {
            GlobalIdentityHandle = "Nobody";
            LogChannelSubscriptions = new List<string>();
        }

        /// <summary>
        /// New up an account with the GlobalIdentityHandle
        /// </summary>
        /// <param name="handle">GlobalIdentityHandle</param>
        public Account(string handle)
        {
            GlobalIdentityHandle = handle;
            LogChannelSubscriptions = new List<string>();
        }

        /// <summary>
        /// New up an account with the GlobalIdentityHandle and the log streams it wants to subscribe to
        /// </summary>
        /// <param name="handle">GlobalIdentityHandle</param>
        /// <param name="logSubscriptions">| delimeted list of log channel names</param>
        public Account(string handle, string logSubscriptions)
        {
            GlobalIdentityHandle = handle;
            LogChannelSubscriptions = logSubscriptions.Split('|');
        }

        /// <summary>
        /// ID for the currently selected character for the account to log into the game as
        /// </summary>
        public long CurrentlySelectedCharacter { get; set; }

        /// <summary>
        /// List of log channel names subscribed to
        /// </summary>
        public IList<string> LogChannelSubscriptions { get; set; }

        /// <summary>
        /// For EF purposes
        /// </summary>
        public string LogSubs
        {
            get { return string.Join(",", LogChannelSubscriptions); }
            set { LogChannelSubscriptions = value.Split(',').ToList(); }
        }

        /// <summary>
        /// List of valid characters this account owns
        /// </summary>
        private IList<ICharacter> _characters;

        /// <summary>
        /// List of valid characters this account owns
        /// </summary>
        public IList<ICharacter> Characters
        {
            get
            {
                if (_characters == null)
                {
                    IEnumerable<ICharacter> returnValue = DataWrapper.GetAllBySharedKey<ICharacter>("AccountHandle", GlobalIdentityHandle);
                    _characters = returnValue.ToList();
                }

                //Cause there are none
                if (_characters == null)
                    _characters = new List<ICharacter>();

                return _characters;
            }
            set
            {
                _characters = value.ToList();
            }
        }

        /// <summary>
        /// Add a character to this account
        /// </summary>
        /// <param name="newChar">the character data to add</param>
        /// <returns>errors or Empty if successful</returns>
        public string AddCharacter(ICharacter newChar)
        {
            IEnumerable<ICharacter> systemChars = DataWrapper.GetAll<ICharacter>();

            if (systemChars.Any(ch => ch.Name.Equals(newChar.Name) && newChar.SurName.Equals(newChar.SurName)))
                return "A character with that name already exists, please choose another.";

            newChar.AccountHandle = GlobalIdentityHandle;
            newChar.Create();

            Characters.Add(newChar);

            return string.Empty;
        }

        /// <summary>
        /// Get an account by its GlobalIdentityHandle
        /// </summary>
        /// <param name="handle">GlobalIdentityHandle to get</param>
        /// <returns>the account</returns>
        public static IAccount GetByHandle(string handle)
        {
            var parms = new Dictionary<string, object>();

            var sql = "select * from [dbo].[Accounts] where GlobalIdentityHandle = @handle";

            parms.Add("handle", handle);

            IAccount account = null;
            try
            {
                var ds = SqlWrapper.RunDataset(sql, CommandType.Text, parms);

                if (ds.Rows != null)
                {
                    foreach (DataRow dr in ds.Rows)
                        account = Fill(dr);
                }
            }
            catch (Exception ex)
            {
                LoggingUtility.LogError(ex);
            }

            return account;
        }

        /// <summary>
        /// Fill an account from the database
        /// </summary>
        /// <param name="dr">the data row to fill from</param>
        /// <returns>the account</returns>
        private static IAccount Fill(DataRow dr)
        {
            string outHandle = DataUtility.GetFromDataRow<string>(dr, "GlobalIdentityHandle");
            string outLogSubs = DataUtility.GetFromDataRow<string>(dr, "LogChannelSubscriptions");
            
            return new Account(outHandle, outLogSubs);
        }
    }
}
