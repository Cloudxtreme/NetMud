﻿using NetMud.Communication;
using System;
using System.Collections.Generic;
using System.Text;

namespace NetMud.Telnet
{
    /// <summary>
    /// Partial class for telnet channel details
    /// </summary>
    public abstract class Channel : IChannelType
    {
        /// <summary>
        /// The format the cachekeys for the comms objects take
        /// </summary>
        public string cacheKeyFormat { get { return "LiveTelnet.{0}"; } }

        /// <summary>
        /// What type of connection the player has
        /// </summary>
        public ConnectionType ConnectedBy { get { return ConnectionType.Telnet; } }

        /// <summary>
        /// Adding a "new line" to the output 
        /// </summary>
        private const string BumperElement = "\n\r";

        /// <summary>
        /// Encapsulate output lines for display to a client
        /// </summary>
        /// <param name="lines">the text lines to encapsulate</param>
        /// <returns>a single string blob of all the output encapsulated</returns>
        public string EncapsulateOutput(IEnumerable<string> lines)
        {
            var returnString = new StringBuilder();

            foreach (var line in lines)
                returnString.AppendFormat(EncapsulateOutput(line));

            return returnString.ToString();
        }

        /// <summary>
        /// Encapsulates a string for output to a client
        /// </summary>
        /// <param name="str">the string to encapsulate</param>
        /// <returns>the encapsulated output</returns>
        public string EncapsulateOutput(string str)
        {
            if (!string.IsNullOrWhiteSpace(str))
                return String.Format("{1}{0}", BumperElement, str);
            else
                return BumperElement; //blank strings mean carriage returns
        }
    }
}
