﻿using System.Collections.Generic;

namespace NetMud.DataStructure.Behaviors.Rendering
{
    /// <summary>
    /// Framework for things that can be accessed by the Help command
    /// </summary>
    public interface IHelpful
    {
        /// <summary>
        /// the text to render when Help targets this
        /// </summary>
        /// <returns>the help output</returns>
        IEnumerable<string> RenderHelpBody();
    }
}
