﻿using NetMud.DataStructure.Base.Supporting;
using System;
using System.Collections.Generic;
using System.Text;

namespace NetMud.Physics
{
    /// <summary>
    /// Render engine for dimensional models
    /// </summary>
    public static class Render
    {
        /// <summary>
        /// Flattens a dimensional model for display
        /// </summary>
        /// <param name="model">the model to flatten</param>
        /// <param name="pitch">rotation on the z-axis</param>
        /// <param name="yaw">rotation on the Y-axis</param>
        /// <param name="roll">rotation on the x-axis</param>
        /// <returns>the flattened view</returns>
        public static string FlattenModel(IDimensionalModelData model, short pitch, short yaw, short roll)
        {
            switch(model.ModelType)
            {
                case DimensionalModelType.None:
                    return String.Empty;
                case DimensionalModelType.Flat:
                    return FlattenFlatModel(model);
                case DimensionalModelType.ThreeD:
                    break; //let it through
            }

            var flattenedModel = new StringBuilder();

            /*
             * We start by looking at the "front" of the model which starts at Y=11, Z=1, X=11 (the upper most left corner node) and contains all the Z=1 nodes of the entire thing
             * 
             * YAW = pivot on X
             * Positive Yaw rotates the object counter-clockwise
             * yaw 1-10 - length = model xAxis 11-1, height = yAxis 11-1, depth = zAxis 1-11
             * yaw 11-21 - length = model zAxis 1-11, height = yAxis 11-1, depth = xAxis 1-11
             * yaw 22-32 - length = model xAxis 1-11, height = yAxis 11-1, depth = zAxis 11-1
             * yaw 33-43 - length = model zAxis 11-1, height = yAxis 11-1, depth = xAxis 11-1
             * 
             * PITCH = pivot on Z
             * Positive pitch rotates the object forward and back
             * pitch 1-10 - length = model xAxis 11-1, height = yAxis 11-1, depth = zAxis 1-11
             * pitch 11-21 - length = model xAxis 11-1, height = zAxis 1-11, depth = yAxis 1-11
             * pitch 22-32 - length = model xAxis 11-1, height = yAxis 1-11, depth = zAxis 11-1
             * pitch 33-43 - length = model xAxis 11-1, height = zAxis 11-1, depth = yAxis 11-1
             * 
             * ROLL = pivot on Y
             * Positive roll "spins" the object diagonally
             * roll 1-10 - length = model xAxis 11-1, height = yAxis 11-1, depth = zAxis 1-11
             * roll 11-21 - length = model yAxis 1-11, height = xAxis 1-1, depth = zAxis 1-11
             * roll 22-32 - length = model xAxis 1-11, height = yAxis 1-1, depth = zAxis 1-11
             * roll 33-43 - length = model yAxis 11-1, height = xAxis 11-1, depth = zAxis 1-11
             * 
             */

            //Figure out the change. We need to "advance" by Length and Height here (where as the "find behind node" function advances Depth only)
            var heightChanges = new short[] { 0, 0, 0 }; // X, Y, Z
            var lengthChanges = new short[] { 0, 0, 0 }; // X, Y, Z

            if (yaw > 0)
            {
                if (yaw <= 10)
                {
                    heightChanges[1]--;
                    lengthChanges[0]--;
                }
                else if (yaw <= 21)
                {
                    heightChanges[1]--;
                    lengthChanges[2]++;
                }
                else if (yaw <= 32)
                {
                    heightChanges[1]++;
                    lengthChanges[0]--;
                }
                else
                {
                    heightChanges[1]--;
                    lengthChanges[2]--;
                }
            }

            if (pitch > 0)
            {
                if (pitch <= 10)
                {
                    heightChanges[1]--;
                    lengthChanges[0]--;
                }
                else if (pitch <= 21)
                {
                    heightChanges[2]++;
                    lengthChanges[0]--;
                }
                else if (pitch <= 32)
                {
                    heightChanges[1]++;
                    lengthChanges[0]--;
                }
                else
                {
                    heightChanges[2]--;
                    lengthChanges[0]--;
                }
            }

            if (roll > 0)
            {
                if (roll <= 10)
                {
                    heightChanges[1]--;
                    lengthChanges[0]--;
                }
                else if (roll <= 21)
                {
                    heightChanges[0]++;
                    lengthChanges[1]++;
                }
                else if (roll <= 32)
                {
                    heightChanges[1]++;
                    lengthChanges[0]++;
                }
                else
                {
                    heightChanges[0]--;
                    lengthChanges[1]--;
                }
            }

            if (roll == 0 && yaw == 0 && pitch == 0)
            {
                heightChanges[1]--;
                lengthChanges[0]--;
            }

            if (heightChanges[0] > 1)
                heightChanges[0] = 1;
            if (heightChanges[0] < -1)
                heightChanges[0] = -1;

            if (heightChanges[1] > 1)
                heightChanges[1] = 1;
            if (heightChanges[1] < -1)
                heightChanges[1] = -1;

            if (heightChanges[2] > 1)
                heightChanges[2] = 1;
            if (heightChanges[2] < -1)
                heightChanges[2] = -1;

            if (lengthChanges[0] > 1)
                lengthChanges[0] = 1;
            if (lengthChanges[0] < -1)
                lengthChanges[0] = -1;

            if (lengthChanges[1] > 1)
                lengthChanges[1] = 1;
            if (lengthChanges[1] < -1)
                lengthChanges[1] = -1;

            if (lengthChanges[2] > 1)
                lengthChanges[2] = 1;
            if (lengthChanges[2] < -1)
                lengthChanges[2] = -1;

            //figure out the starting point which is like the viewer's pov
            var startVertex = FindStartingVertex(yaw, pitch, roll);

            int startXAxis = startVertex.Item1;
            int startYAxis = startVertex.Item2;
            int startZAxis = startVertex.Item3;

            //load the plane up with blanks
            List<string[]> flattenedPlane = new List<string[]>();
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });

            short xAxis, yAxis, zAxis;
            int xI, yI;
            for (yI = 0; yI < 11; yI++)
            {
                xAxis = (short)(startXAxis + (heightChanges[0] * yI));
                yAxis = (short)(startYAxis + (heightChanges[1] * yI));
                zAxis = (short)(startZAxis + (heightChanges[2] * yI));

                for (xI = 0; xI < 11; xI++)
                {
                    if (xAxis <= 0)
                        xAxis = (short)(11 + xAxis);
                    if (yAxis <= 0)
                        yAxis = (short)(11 + yAxis);
                    if (zAxis <= 0)
                        zAxis = (short)(11 + zAxis);

                    if (xAxis > 11)
                        xAxis = (short)(xAxis - 11);
                    if (yAxis > 11)
                        yAxis = (short)(yAxis - 11);
                    if (zAxis > 11)
                        zAxis = (short)(zAxis - 11);

                    var node = model.GetNode(xAxis, yAxis, zAxis);

                    while (node != null && String.IsNullOrWhiteSpace(flattenedPlane[yI][xI]))
                    {
                        flattenedPlane[yI][xI] = DamageTypeToCharacter(node.Style, xI < 5);

                        node = model.GetNodeBehindNode(node.XAxis, node.YAxis, node.ZAxis, pitch, yaw, roll);
                    }

                    //reset everything to either the proper length start or height start
                    if (lengthChanges[0] != 0)
                        xAxis = (short)(xAxis + lengthChanges[0]);
                    else
                        xAxis = (short)(startXAxis + (heightChanges[0] * yI));

                    if (lengthChanges[1] != 0)
                        yAxis = (short)(yAxis + lengthChanges[1]);
                    else
                        yAxis = (short)(startYAxis + (heightChanges[1] * yI));

                    if (lengthChanges[2] != 0)
                        zAxis = (short)(zAxis + lengthChanges[2]);
                    else
                        zAxis = (short)(startZAxis + (heightChanges[2] * yI));
                }
            }

            flattenedModel.AppendLine();

            //Write out the flattened view to the string builder with line terminators
            foreach (var nodes in flattenedPlane)
                flattenedModel.AppendLine(string.Join("", nodes));

            flattenedModel.AppendLine();

            return flattenedModel.ToString();
        }

        /// <summary>
        /// Converts damage types to characters for use in model rendering
        /// </summary>
        /// <param name="type">the damage type to convert</param>
        /// <param name="reverseCharacter">whether or not to display the left-of-center character</param>
        /// <returns></returns>
        public static string DamageTypeToCharacter(DamageType type, bool leftOfCenter)
        {
            string returnString = " ";

            switch (type)
            {
                default: //also "none" case
                    break;
                case DamageType.Blunt:
                    returnString = "#";
                    break;
                case DamageType.Sharp:
                    if (leftOfCenter)
                        returnString = "/";
                    else
                        returnString = "\\";
                    break;
                case DamageType.Pierce:
                    returnString = "^";
                    break;
                case DamageType.Shred:
                    if (leftOfCenter)
                        returnString = "<";
                    else
                        returnString = ">";
                    break;
                case DamageType.Chop:
                    if (leftOfCenter)
                        returnString = "{";
                    else
                        returnString = "}";
                    break;
                case DamageType.Acidic:
                    returnString = "A";
                    break;
                case DamageType.Base:
                    returnString = "B";
                    break;
                case DamageType.Heat:
                    returnString = "H";
                    break;
                case DamageType.Cold:
                    returnString = "C";
                    break;
                case DamageType.Electric:
                    returnString = "E";
                    break;
                case DamageType.Positronic:
                    returnString = "P";
                    break;
                case DamageType.Endergonic:
                    returnString = "N";
                    break;
                case DamageType.Exergonic:
                    returnString = "X";
                    break;
                case DamageType.Hypermagnetic:
                    returnString = "M";
                    break;
            }

            return returnString;
        }

        /// <summary>
        /// Converts render characters to damage types
        /// </summary>
        /// <param name="chr">actually a string, the character to convert</param>
        /// <returns>the damage type</returns>
        public static DamageType CharacterToDamageType(string chr)
        {
            var returnValue = DamageType.None;

            switch (chr)
            {
                default: //also "none" case
                    break;
                case "#":
                    returnValue = DamageType.Blunt;
                    break;
                case "\\":
                case "/":
                    returnValue = DamageType.Sharp;
                    break;
                case "^":
                    returnValue = DamageType.Pierce;
                    break;
                case "<":
                case ">":
                    returnValue = DamageType.Shred;
                    break;
                case "{":
                case "}":
                    returnValue = DamageType.Chop;
                    break;
                case "A":
                    returnValue = DamageType.Acidic;
                    break;
                case "B":
                    returnValue = DamageType.Base;
                    break;
                case "H":
                    returnValue = DamageType.Heat;
                    break;
                case "C":
                    returnValue = DamageType.Cold;
                    break;
                case "E":
                    returnValue = DamageType.Electric;
                    break;
                case "P":
                    returnValue = DamageType.Positronic;
                    break;
                case "N":
                    returnValue = DamageType.Endergonic;
                    break;
                case "X":
                    returnValue = DamageType.Exergonic;
                    break;
                case "M":
                    returnValue = DamageType.Hypermagnetic;
                    break;
            }

            return returnValue;
        }

        public static Tuple<short, short, short> FindStartingVertex(short yaw, short pitch, short roll)
        {
            //figure out the starting point which is like the viewer's pov
            int startXAxis = 11, startYAxis = 11, startZAxis = 1;

            var heightChanges = new short[] { 0, 0, 0 }; // X, Y, Z
            var lengthChanges = new short[] { 0, 0, 0 }; // X, Y, Z

            if (yaw > 0)
            {
                if (yaw <= 10)
                {
                    heightChanges[1]--;
                    lengthChanges[0]--;
                }
                else if (yaw <= 21)
                {
                    heightChanges[1]--;
                    lengthChanges[2]++;
                }
                else if (yaw <= 32)
                {
                    heightChanges[1]++;
                    lengthChanges[0]--;
                }
                else
                {
                    heightChanges[1]--;
                    lengthChanges[2]--;
                }
            }

            if (pitch > 0)
            {
                if (pitch <= 10)
                {
                    heightChanges[1]--;
                    lengthChanges[0]--;
                }
                else if (pitch <= 21)
                {
                    heightChanges[2]++;
                    lengthChanges[0]--;
                }
                else if (pitch <= 32)
                {
                    heightChanges[1]++;
                    lengthChanges[0]--;
                }
                else
                {
                    heightChanges[2]--;
                    lengthChanges[0]--;
                }
            }

            if (roll > 0)
            {
                if (roll <= 10)
                {
                    heightChanges[1]--;
                    lengthChanges[0]--;
                }
                else if (roll <= 21)
                {
                    heightChanges[0]++;
                    lengthChanges[1]++;
                }
                else if (roll <= 32)
                {
                    heightChanges[1]++;
                    lengthChanges[0]++;
                }
                else
                {
                    heightChanges[0]--;
                    lengthChanges[1]--;
                }
            }

            if (lengthChanges[0] != 0 || heightChanges[0] != 0)
            {
                Math.DivRem(yaw, 10, out startXAxis);
                startXAxis = 11 - startXAxis;
            }

            if (lengthChanges[1] != 0 || heightChanges[1] != 0)
            {
                Math.DivRem(roll, 10, out startYAxis);
                startYAxis = 11 - startYAxis;
            }

            if (lengthChanges[2] != 0 || heightChanges[2] != 0)
            {
                Math.DivRem(pitch, 10, out startZAxis);
                startZAxis = 11 - startZAxis;
            }

            startZAxis = Math.Max(1, startZAxis);
            startYAxis = Math.Max(1, startYAxis);
            startXAxis = Math.Max(1, startXAxis);

            return new Tuple<short, short, short>((short)startXAxis, (short)startYAxis, (short)startZAxis);
        }

        private static string FlattenFlatModel(IDimensionalModelData model)
        {
            var flattenedModel = new StringBuilder();

            //load the plane up with blanks
            List<string[]> flattenedPlane = new List<string[]>();
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });
            flattenedPlane.Add(new string[] { " ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " " });

            short xI, yI;
            for (yI = 0; yI < 11; yI++)
            {
                for (xI = 0; xI < 11; xI++)
                {
                    var node = model.GetNode(xI, yI, 0);

                    flattenedPlane[yI][xI] = DamageTypeToCharacter(node.Style, xI < 5);
                }
            }

            flattenedModel.AppendLine();

            //Write out the flattened view to the string builder with line terminators
            foreach (var nodes in flattenedPlane)
                flattenedModel.AppendLine(string.Join("", nodes));

            flattenedModel.AppendLine();

            return flattenedModel.ToString();
        }
    }
}
