﻿using NetMud.Commands.Attributes;
using NetMud.Communication.Messaging;
using NetMud.DataStructure.Base.System;
using NetMud.DataStructure.Behaviors.Rendering;
using NetMud.DataStructure.SupportingClasses;
using NetMud.Utility;
using NutMud.Commands.Attributes;
using System.Collections.Generic;

namespace NetMud.Commands.EntityManipulation
{
    [CommandKeyword("get", false)]
    [CommandKeyword("take", false)]
    [CommandPermission(StaffRank.Player)]
    [CommandParameter(CommandUsage.Subject, typeof(IEntity), new CacheReferenceType[] { CacheReferenceType.Entity }, false)]
    [CommandParameter(CommandUsage.Target, typeof(IContains), new CacheReferenceType[] { CacheReferenceType.Container }, true)]
    [CommandRange(CommandRangeType.Touch, 0)]
    public class Get : CommandPartial, IHelpful
    {
        /// <summary>
        /// All Commands require a generic constructor
        /// </summary>
        public Get()
        {
            //Generic constructor for all IHelpfuls is needed
        }

        /// <summary>
        /// Executes this command
        /// </summary>
        public override void Execute()
        {
            var sb = new List<string>();
            var thing = (IEntity)Subject;
            var actor = (IContains)Actor;
            IContains place;

            string toRoomMessage = "$A$ gets $S$.";

            if (Target != null)
            {
                place = (IContains)Target;
                toRoomMessage = "$A$ gets $S$ from $T$."; 
                sb.Add("You get $S$ from $T$.");
            }
            else
            {
                place = (IContains)OriginLocation;
                sb.Add("You get $S$.");
            }

            place.MoveFrom(thing);
            actor.MoveInto(thing);

            var messagingObject = new MessageCluster(sb, new string[] { }, new string[] { }, new string[] { toRoomMessage }, new string[] { });

            messagingObject.ExecuteMessaging(Actor, thing, (IEntity)Target, OriginLocation, null);
        }

        /// <summary>
        /// Renders syntactical help for the command, invokes automatically when syntax is bungled
        /// </summary>
        /// <returns>string</returns>
        public override IEnumerable<string> RenderSyntaxHelp()
        {
            var sb = new List<string>();

            sb.Add("Valid Syntax: get &lt;object&gt;");
            sb.Add("take &lt;object&gt;".PadWithString(14, "&nbsp;", true));
            sb.Add("get &lt;object&gt; &lt;container&gt;".PadWithString(14, "&nbsp;", true));
            sb.Add("take &lt;object&gt; &lt;container&gt;".PadWithString(14, "&nbsp;", true));
            return sb;
        }

        /// <summary>
        /// Renders the help text
        /// </summary>
        /// <returns>string</returns>
        public IEnumerable<string> RenderHelpBody()
        {
            var sb = new List<string>();

            sb.Add(string.Format("Takes an object from a container or if unspecified attempts to take it from the room you are in."));

            return sb;
        }
    }
}
