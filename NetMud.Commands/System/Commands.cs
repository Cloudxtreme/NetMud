﻿using NetMud.Commands.Attributes;
using NetMud.Communication.Messaging;
using NetMud.DataStructure.Base.Entity;
using NetMud.DataStructure.Base.EntityBackingData;
using NetMud.DataStructure.Base.System;
using NetMud.DataStructure.Behaviors.Rendering;
using NetMud.DataStructure.SupportingClasses;
using NetMud.Utility;
using NutMud.Commands.Attributes;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Text;

namespace NetMud.Commands.System
{
    [CommandKeyword("commands", false)]
    [CommandPermission(StaffRank.Player)]
    [CommandRange(CommandRangeType.Touch, 0)]
    public class Commands : CommandPartial, IHelpful
    {
        /// <summary>
        /// All Commands require a generic constructor
        /// </summary>
        public Commands()
        {
            //Generic constructor for all IHelpfuls is needed
        }

        public override void Execute()
        {
            //NPCs dont need to use this
            if (!Actor.GetType().GetInterfaces().Contains(typeof(IPlayer)))
                return;

            var returnStrings = new List<string>();
            var sb = new StringBuilder();

            var commandsAssembly = Assembly.GetAssembly(typeof(CommandParameterAttribute));

            var loadedCommands = commandsAssembly.GetTypes().Where(t => t.GetInterfaces().Contains(typeof(ICommand)));

            loadedCommands = loadedCommands.Where(comm => comm.GetCustomAttributes<CommandPermissionAttribute>().Any(att => att.MinimumRank <= ((ICharacter)Actor.DataTemplate).GamePermissionsRank));

            returnStrings.Add("Commands:");

            foreach (var commandName in loadedCommands.Select(comm => comm.Name))
                sb.Append(commandName + ", ");

            if(sb.Length > 0)
                sb.Length -= 2;

            returnStrings.Add(sb.ToString());

            var messagingObject = new MessageCluster(returnStrings, new string[] { }, new string[] { }, new string[] { }, new string[] { });

            messagingObject.ExecuteMessaging(Actor, null, null, null, null);
        }

        public override IEnumerable<string> RenderSyntaxHelp()
        {
            var sb = new List<string>();

            sb.Add("Valid Syntax: commands");

            return sb;
        }

        public IEnumerable<string> RenderHelpBody()
        {
            var sb = new List<string>();

            sb.Add(string.Format("Commands lists possible commands for you to use in-game."));

            return sb;
        }
    }
}
