﻿using NetMud.DataStructure.Behaviors.Rendering;
using System.Collections.Generic;

namespace NetMud.DataStructure.Base.System
{
    /// <summary>
    /// Framework interface for commands
    /// </summary>
    public interface ICommand
    {
        /// <summary>
        /// Execute the command's actions
        /// </summary>
        void Execute();

        /// <summary>
        /// Renders syntactical help for command parsing
        /// </summary>
        /// <returns>help output</returns>
        IEnumerable<string> RenderSyntaxHelp();

        /* 
         * Syntax:
         *      command <subject> <target> <supporting>
         *  Location is derived from context
         *  Surroundings is derived from location
         */

        /// <summary>
        /// Acting entity that issued this command
        /// </summary>
        IActor Actor { get; set; }

        /// <summary>
        /// The subject for the command being issued
        /// </summary>
        object Subject { get; set; }

        /// <summary>
        /// The target for the command being issued
        /// </summary>
        object Target { get; set; }

        /// <summary>
        /// Any supporting entity for the command
        /// </summary>
        object Supporting { get; set; }

        /// <summary>
        /// Location the Actor was in when command was issued
        /// </summary>
        ILocation OriginLocation { get; set; }

        /// <summary>
        /// Any surrounding locations to the origin or target locations for the command
        /// </summary>
        IEnumerable<ILocation> Surroundings { get; set; }
    }
}
