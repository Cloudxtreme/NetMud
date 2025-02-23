﻿CREATE TABLE [dbo].[PathwayData]
(
    [Id]						BIGINT			IDENTITY (1, 1) NOT NULL,
    [Created]					DATETIME		DEFAULT (getutcdate()) NOT NULL,
    [LastRevised]				DATETIME		DEFAULT (getutcdate()) NOT NULL,
    [Name]						VARCHAR (200)	NOT NULL,
	[DegreesFromNorth]			INT				DEFAULT((0)) NOT NULL,
	[ToLocationID]				VARCHAR (MAX)	NOT NULL,
	[ToLocationType]			VARCHAR (MAX)	NOT NULL,
	[FromLocationID]			VARCHAR (MAX)	NOT NULL,
	[FromLocationType]			VARCHAR (MAX)	NOT NULL,
	[MessageToActor]			VARCHAR (MAX)	NOT NULL,
	[MessageToOrigin]			VARCHAR (MAX)	NOT NULL,
	[MessageToDestination]		VARCHAR (MAX)	NOT NULL,
	[AudibleToSurroundings]		VARCHAR (MAX)	NOT NULL,
	[VisibleToSurroundings]		VARCHAR (MAX)	NOT NULL,
	[AudibleStrength]			INT				DEFAULT((3)) NOT NULL,
	[VisibleStrength]			INT				DEFAULT((1)) NOT NULL,
	[DimensionalModelLength]	INT				NOT NULL DEFAULT (-1),
	[DimensionalModelHeight]	INT				NOT NULL DEFAULT (-1),
	[DimensionalModelWidth]		INT				NOT NULL DEFAULT (-1),
	[DimensionalModelMaterialCompositions]		VARCHAR (MAX)	DEFAULT ('{}') NOT NULL,
	[DimensionalModelID]		BIGINT			NOT NULL
)
