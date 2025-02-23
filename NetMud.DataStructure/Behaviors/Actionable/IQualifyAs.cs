﻿using System.Collections.Generic;
namespace NetMud.DataStructure.Behaviors.Actionable
{
    /// <summary>
    /// For commands/events, inanimate qualifies for crafting/spell/command materials.
    /// Super generic stuff is stored here as strings, it needs to be handled on an individual level
    /// </summary>
    public interface IQualifyAs
    {
        Qualification QualificationType { get; set; }

        IDictionary<string, string> SupportingData { get; set; }

        string Serialize();

        IQualifyAs Deserialize();
    }

    public enum Qualification
    {
        PadLock
    }
}
