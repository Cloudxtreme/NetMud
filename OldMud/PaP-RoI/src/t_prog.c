/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

/* beginning of conversions... ouch... - TRI */
/* -- Conversions of all mob_prog.c over to t_prog.c (trap_prog) by TRI  */
/* Including new triggers - ick... annoying...                           */
/* For Objs/exits/rooms to be able to do ch/mob type things, a mob must  */
/* be loaded to perform the actions/commands of the trap_progs belonging */
/* to that obj/room/exit -- for this, a standard "SuperMob" (which I put */
/* in limbo.are) is used... to make it appear to be the exit/room/obj,   */
/* that obj/room/exit's name is str_dup'd into the cloned SuperMob's     */
/* player_name, etc field						 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"

/*
 * Local function prototypes
 */

char *	tprg_next_command	args( ( char* clist ) );
bool	tprg_seval		args( ( char* lhs, char* opr, char* rhs ) );
bool	tprg_veval		args( ( int lhs, char* opr, int rhs ) );
bool	tprg_do_ifchck		args( ( char* ifchck, ROOM_INDEX_DATA *room,
				       OBJ_INDEX_DATA *tObj,
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo, CHAR_DATA* rndm ) );
char *	tprg_process_if	args( ( char* ifchck, char* com_list, 
				       ROOM_INDEX_DATA *room,
				       OBJ_INDEX_DATA *tObj, 
				       CHAR_DATA* actor,
				       OBJ_DATA* obj, void* vo,
				       CHAR_DATA* rndm ) );
void	tprg_translate		args( ( char ch, char* t, 
				       ROOM_INDEX_DATA *room,
				       OBJ_INDEX_DATA *tObj,
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo, CHAR_DATA* rndm ) );
void	tprg_process_cmnd	args( ( char* cmnd, ROOM_INDEX_DATA *room,
				       OBJ_INDEX_DATA *tObj, 
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo, CHAR_DATA* rndm ) );
void	tprg_driver		args( ( char* com_list, ROOM_INDEX_DATA *room,
				       OBJ_INDEX_DATA *tObj,
				       CHAR_DATA* actor, OBJ_DATA* obj,
				       void* vo ) );

/***************************************************************************
 * Local function code and brief comments.
 */

/* if you dont have these functions, you damn well should... */

#ifdef DUNNO_STRSTR
char * strstr(s1,s2) const char *s1; const char *s2;
{
  char *cp;
  int i,j=strlen(s1)-strlen(s2),k=strlen(s2);
  if(j<0)
    return NULL;
  for(i=0; i<=j && strncmp(s1++,s2, k)!=0; i++);
  return (i>j) ? NULL : (s1-1);
}
#endif

/* Used to get sequential lines of a multi line string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *tprg_next_command( char *clist )
{
  char *pointer;

  pointer = clist;
  while ( *pointer != '\n' && *pointer != '\0' )
    pointer++;
  if ( *pointer == '\n' )
    *pointer++ = '\0';
  if ( *pointer == '\r' )
    *pointer++ = '\0';

  return pointer;

}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
bool tprg_seval( char *lhs, char *opr, char *rhs )
{

  if ( !str_cmp( opr, "==" ) )
    return ( bool )( !str_cmp( lhs, rhs ) );
  if ( !str_cmp( opr, "!=" ) )
    return ( bool )( str_cmp( lhs, rhs ) );
  if ( !str_cmp( opr, "/" ) )
    return ( bool )( !str_infix( rhs, lhs ) );
  if ( !str_cmp( opr, "!/" ) )
    return ( bool )( str_infix( rhs, lhs ) );

  bug ( "Improper TRAPprog operator\n\r", 0 );
  return 0;

}

bool tprg_veval( int lhs, char *opr, int rhs )
{

  if ( !str_cmp( opr, "==" ) )
    return ( lhs == rhs );
  if ( !str_cmp( opr, "!=" ) )
    return ( lhs != rhs );
  if ( !str_cmp( opr, ">" ) )
    return ( lhs > rhs );
  if ( !str_cmp( opr, "<" ) )
    return ( lhs < rhs );
  if ( !str_cmp( opr, ">=" ) )
    return ( lhs <= rhs );
  if ( !str_cmp( opr, ">=" ) )
    return ( lhs >= rhs );
  if ( !str_cmp( opr, "&" ) )
    return ( lhs & rhs );
  if ( !str_cmp( opr, "|" ) )
    return ( lhs | rhs );

  bug ( "Improper TRAPprog operator\n\r", 0 );
  return 0;

}

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 * If there are errors, then return -1 otherwise return boolean 1,0
 */
bool tprg_do_ifchck( char *ifchck, ROOM_INDEX_DATA *room, OBJ_INDEX_DATA *tObj,
            CHAR_DATA *actor, OBJ_DATA *obj, void *vo, CHAR_DATA *rndm)
{

  char buf[ MAX_INPUT_LENGTH ];
  char arg[ MAX_INPUT_LENGTH ];
  char opr[ MAX_INPUT_LENGTH ];
  char val[ MAX_INPUT_LENGTH ];
  CHAR_DATA *vict = (CHAR_DATA *) vo;
  OBJ_DATA *v_obj = (OBJ_DATA  *) vo;
  char     *bufpt = buf;
  char     *argpt = arg;
  char     *oprpt = opr;
  char     *valpt = val;
  char     *point = ifchck;
  int       lhsvl;
  int       rhsvl;

  if ( *point == '\0' ) 
    {
      if ( room )
      bug( "Room: %d null ifchck", room->vnum );
      if ( tObj )
      bug( "Obj: %d null ifchck", tObj->vnum );
      return -1;
    }   
  /* skip leading spaces */
  while ( *point == ' ' )
    point++;

  /* get whatever comes before the left paren.. ignore spaces */
  while ( *point != '(' ) 
    if ( *point == '\0' ) 
      {
      /*
	bug ( "Trap: %d ifchck syntax error (missing opening '(' )", mob->pIndexData->vnum );  */
	return -1;
      }   
    else
      if ( *point == ' ' )
	point++;
      else 
	*bufpt++ = *point++; 

  *bufpt = '\0';
  point++;

  /* get whatever is in between the parens.. ignore spaces */
  while ( *point != ')' ) 
    if ( *point == '\0' ) 
      {
      /*
	bug ( "Mob: %d ifchck syntax error (missing closing ')' )", mob->pIndexData->vnum );  */
	return -1;
      }   
    else
      if ( *point == ' ' )
	point++;
      else 
	*argpt++ = *point++; 

  *argpt = '\0';
  point++;

  /* check to see if there is an operator */
  while ( *point == ' ' )
    point++;
  if ( *point == '\0' ) 
    {
      *opr = '\0';
      *val = '\0';
    }   
  else /* there should be an operator and value, so get them */
    {
      while ( ( *point != ' ' ) && ( !isalnum( *point ) ) ) 
	if ( *point == '\0' ) 
	  {
	  /*
	    bug ( "Mob: %d ifchck operator without value",
		 mob->pIndexData->vnum );  */
	    return -1;
	  }   
	else
	  *oprpt++ = *point++; 

      *oprpt = '\0';
 
      /* finished with operator, skip spaces and then get the value */
      while ( *point == ' ' )
	point++;
      for( ; ; )
	{
	  if ( ( *point != ' ' ) && ( *point == '\0' ) )
	    break;
	  else
	    *valpt++ = *point++; 
	}

      *valpt = '\0';
    }
  bufpt = buf;
  argpt = arg;
  oprpt = opr;
  valpt = val;

  /* Ok... now buf contains the ifchck, arg contains the inside of the
   *  parentheses, opr contains an operator if one is present, and val
   *  has the value if an operator was present.
   *  So.. basically use if statements and run over all known ifchecks
   *  Once inside, use the argument and expand the lhs. Then if need be
   *  send the lhs,opr,rhs off to be evaluated.
   */

  if ( !str_cmp( buf, "rand" ) )
    {
      return ( number_percent() <= atoi(arg) );
    }

  if ( !str_cmp( buf, "ispc" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return 0;
	case 'n': if ( actor )
 	             return ( !IS_NPC( actor ) );
	          else return -1;
	case 't': if ( vict )
                     return ( !IS_NPC( vict ) );
	          else return -1;
	case 'r': if ( rndm )
                     return ( !IS_NPC( rndm ) );
	          else return -1;
	default:
	/*
	  sprintf( log_buf, "Mob: %d bad argument to 'ispc' (%s)", 
	   	mob->pIndexData->vnum, arg ); 
	  bug ( log_buf, 0 ); */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isnpc" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'i': return 1;
	case 'n': if ( actor )
	             return IS_NPC( actor );
	          else return -1;
	case 't': if ( vict )
                     return IS_NPC( vict );
	          else return -1;
	case 'r': if ( rndm )
	             return IS_NPC( rndm );
	          else return -1;
	default:
	/*
	  sprintf(log_buf, "Mob: %d bad argument to 'isnpc' (%s)", mob->pIndexData->vnum,
	   arg ); 
	  bug (log_buf, 0 ); */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isclan" ) )
  {
    switch ( arg[1] )
    {
    case 'n' : if (actor)
      if (!IS_NPC( actor ) )
      {
	lhsvl = actor->clan;
	rhsvl = atoi(val);
	return tprg_veval( lhsvl, opr,  rhsvl );
      }
      else return -1;
    default:
    /*
      sprintf(log_buf, "Mob: %d bad arg to ifchck 'isclan' (%s)", mob->pIndexData->vnum,
	      arg );
      bug( log_buf, 0 ); */
      return -1;
    }	
  }
                    
  if ( !str_cmp( buf, "isframed" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	             return IS_FRAMED( actor );
	          else return -1;
	case 't': if ( vict )
	             return IS_FRAMED( vict );
	          else return -1;
	case 'r': if ( rndm )
	             return IS_FRAMED( rndm );
	          else return -1;
	default:
	/*
	  sprintf(log_buf,  "Mob: %d bad argument to 'isframed' (%s)", mob->pIndexData->vnum,
	  arg ); 
	  bug( log_buf, 0 ); */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isfight" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	             return ( actor->fighting ) ? 1 : 0;
	          else return -1;
	case 't': if ( vict )
	             return ( vict->fighting ) ? 1 : 0;
	          else return -1;
	case 'r': if ( rndm )
	             return ( rndm->fighting ) ? 1 : 0;
	          else return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'isfight'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isimmort" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	             return ( get_trust( actor ) > LEVEL_IMMORTAL );
  	          else return -1;
	case 't': if ( vict )
	             return ( get_trust( vict ) > LEVEL_IMMORTAL );
                  else return -1;
	case 'r': if ( rndm )
	             return ( get_trust( rndm ) > LEVEL_IMMORTAL );
                  else return -1;
	default:
/*	  bug ( "Mob: %d bad argument to 'isimmort'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "ischarmed" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	             return (bool) IS_AFFECTED( actor, AFF_CHARM );
	          else return -1;
	case 't': if ( vict )
	             return (bool) IS_AFFECTED( vict, AFF_CHARM );
	          else return -1;
	case 'r': if ( rndm )
	             return (bool) IS_AFFECTED( rndm, AFF_CHARM );
	          else return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'ischarmed'",
	       mob->pIndexData->vnum ); */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isfollow" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
			  && mob->master->in_room == mob->in_room ); */
	case 'n': if ( actor )
	             return ( actor->master != NULL
			     && actor->master->in_room == actor->in_room );
	          else return -1;
	case 't': if ( vict )
	             return ( vict->master != NULL
			     && vict->master->in_room == vict->in_room );
	          else return -1;
	case 'r': if ( rndm )
	             return ( rndm->master != NULL
			     && rndm->master->in_room == rndm->in_room );
	          else return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'isfollow'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "isaffected" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	             return ( actor->affected_by & atoi( arg ) );
	          else return -1;
	case 't': if ( vict )
	             return ( vict->affected_by & atoi( arg ) );
	          else return -1;
	case 'r': if ( rndm )
	             return ( rndm->affected_by & atoi( arg ) );
	          else return -1;
	default:
/*	  bug ( "Mob: %d bad argument to 'isaffected'",
	       mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "hitprcnt" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	          rhsvl = atoi( val );
         	  return tprg_veval( lhsvl, opr, rhsvl ); */
	case 'n': if ( actor )
	          {
		    lhsvl = actor->hit / actor->max_hit;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->hit / vict->max_hit;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->hit / rndm->max_hit;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'hitprcnt'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "inroom" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	          rhsvl = atoi(val);
	          return tprg_veval( lhsvl, opr, rhsvl ); */
	case 'n': if ( actor )
	          {
		    lhsvl = actor->in_room->vnum;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->in_room->vnum;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->in_room->vnum;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'inroom'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "sex" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	          {
		    lhsvl = actor->sex;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->sex;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->sex;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'sex'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "position" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	          {
		    lhsvl = actor->position;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->position;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->position;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'position'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "level" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	          {
		    lhsvl = get_trust( actor );
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else 
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = get_trust( vict );
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = get_trust( rndm );
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'level'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "class" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	          {
		    lhsvl = actor->class;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else 
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->class;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->class;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'class'", mob->pIndexData->vnum ); */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "goldamt" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	          {
		    lhsvl = actor->money.gold;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    lhsvl = vict->money.gold;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'r': if ( rndm )
	          {
		    lhsvl = rndm->money.gold;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'goldamt'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objtype" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->item_type;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	         else
		   return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->item_type;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'objtype'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval0" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->value[0];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->value[0];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else 
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'objval0'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval1" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->value[1];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->value[1];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'objval1'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval2" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->value[2];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->value[2];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'objval2'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "objval3" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'o': if ( obj )
	          {
		    lhsvl = obj->value[3];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj ) 
	          {
		    lhsvl = v_obj->value[3];
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
	/*
	  bug ( "Mob: %d bad argument to 'objval3'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "number" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	          {
		    if IS_NPC( actor )
		    {
		      lhsvl = actor->pIndexData->vnum;
		      rhsvl = atoi( val );
		      return tprg_veval( lhsvl, opr, rhsvl );
		    }
		  }
	          else
		    return -1;
	case 't': if ( vict )
	          {
		    if IS_NPC( actor )
		    {
		      lhsvl = vict->pIndexData->vnum;
		      rhsvl = atoi( val );
		      return tprg_veval( lhsvl, opr, rhsvl );
		    }
		  }
                  else
		    return -1;
	case 'r': if ( rndm )
	          {
		    if IS_NPC( rndm )
		    {
		      lhsvl = rndm->pIndexData->vnum;
		      rhsvl = atoi( val );
		      return tprg_veval( lhsvl, opr, rhsvl );
		    }
		  }
	         else return -1;
	case 'o': if ( obj )
	          {
		    lhsvl = obj->pIndexData->vnum;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	case 'p': if ( v_obj )
	          {
		    lhsvl = v_obj->pIndexData->vnum;
		    rhsvl = atoi( val );
		    return tprg_veval( lhsvl, opr, rhsvl );
		  }
	          else
		    return -1;
	default:
/*	  bug ( "Mob: %d bad argument to 'number'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  if ( !str_cmp( buf, "name" ) )
    {
      switch ( arg[1] )  /* arg should be "$*" so just get the letter */
	{
	case 'n': if ( actor )
	            return tprg_seval( actor->name, opr, val );
	          else
		    return -1;
	case 't': if ( vict )
	            return tprg_seval( vict->name, opr, val );
	          else
		    return -1;
	case 'r': if ( rndm )
	            return tprg_seval( rndm->name, opr, val );
	          else
		    return -1;
	case 'o': if ( obj )
	            return tprg_seval( obj->name, opr, val );
	          else
		    return -1;
	case 'p': if ( v_obj )
	            return tprg_seval( v_obj->name, opr, val );
	          else
		    return -1;
	default:
/*	  bug ( "Mob: %d bad argument to 'name'", mob->pIndexData->vnum );  */
	  return -1;
	}
    }

  /* Ok... all the ifchcks are done, so if we didnt find ours then something
   * odd happened.  So report the bug and abort the MOBprogram (return error)
   */
   /*
  sprintf( log_buf, "Mob: %d unkown ifchck (%s)", mob->pIndexData->vnum,
         buf );
  bug( log_buf, 0 ); */
  return -1;

}
/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
char *tprg_process_if( char *ifchck, char *com_list, ROOM_INDEX_DATA *room,
		       OBJ_INDEX_DATA *tObj,
		       CHAR_DATA *actor, OBJ_DATA *obj, void *vo,
		       CHAR_DATA *rndm )
{
 static char null[ 1 ];
 char buf[ MAX_INPUT_LENGTH ];
 char *morebuf = '\0';
 char    *cmnd = '\0';
 bool loopdone = FALSE;
 bool     flag = FALSE;
 int  legal;

 null[0] = '\0';

 /* check for trueness of the ifcheck */
 if ( ( legal = tprg_do_ifchck( ifchck, room, tObj, actor, obj, vo, rndm ) ) )
   if ( legal == 1 )
     flag = TRUE;
   else
     return null;

 while( loopdone == FALSE ) /*scan over any existing or statements */
 {
     cmnd     = com_list;
     com_list = tprg_next_command( com_list );
     while ( *cmnd == ' ' )
       cmnd++;
     if ( *cmnd == '\0' )
     {
     /*
	 bug ( "Mob: %d no commands after IF/OR", mob->pIndexData->vnum );  */
	 return null;
     }
     morebuf = one_argument( cmnd, buf );
     if ( !str_cmp( buf, "or" ) )
     {
	 if ( ( legal = tprg_do_ifchck( morebuf,room,tObj,actor,obj,vo,rndm ) ) )
	   if ( legal == 1 )
	     flag = TRUE;
	   else
	     return null;
     }
     else
       loopdone = TRUE;
 }
 
 if ( flag )
   for ( ; ; ) /*ifcheck was true, do commands but ignore else to endif*/ 
   {
       if ( !str_cmp( buf, "if" ) )
       { 
	   com_list = tprg_process_if(morebuf,com_list,room,tObj,actor,obj,vo,rndm);
	   while ( *cmnd==' ' )
	     cmnd++;
	   if ( *com_list == '\0' )
	     return null;
	   cmnd     = com_list;
	   com_list = tprg_next_command( com_list );
	   morebuf  = one_argument( cmnd,buf );
	   continue;
       }
       if ( !str_cmp( buf, "break" ) )
	 return null;
       if ( !str_cmp( buf, "endif" ) )
	 return com_list; 
       if ( !str_cmp( buf, "else" ) ) 
       {
	   while ( str_cmp( buf, "endif" ) ) 
	   {
	       cmnd     = com_list;
	       com_list = tprg_next_command( com_list );
	       while ( *cmnd == ' ' )
		 cmnd++;
	       if ( *cmnd == '\0' )
	       {
	       /*
		   bug ( "Mob: %d missing endif after else",
			mob->pIndexData->vnum ); */
		   return null;
	       }
	       morebuf = one_argument( cmnd,buf );
	   }
	   return com_list; 
       }
       tprg_process_cmnd( cmnd, room, tObj, actor, obj, vo, rndm );
       cmnd     = com_list;
       com_list = tprg_next_command( com_list );
       while ( *cmnd == ' ' )
	 cmnd++;
       if ( *cmnd == '\0' )
       {
       /*
           bug ( "Mob: %d missing else or endif", mob->pIndexData->vnum );  */
           return null;
       }
       morebuf = one_argument( cmnd, buf );
   }
 else /*false ifcheck, find else and do existing commands or quit at endif*/
   {
     while ( ( str_cmp( buf, "else" ) ) && ( str_cmp( buf, "endif" ) ) )
       {
	 cmnd     = com_list;
	 com_list = tprg_next_command( com_list );
	 while ( *cmnd == ' ' )
	   cmnd++;
	 if ( *cmnd == '\0' )
	   {
	   /*
	     bug ( "Mob: %d missing an else or endif",
		  mob->pIndexData->vnum );  */
	     return null;
	   }
	 morebuf = one_argument( cmnd, buf );
       }

     /* found either an else or an endif.. act accordingly */
     if ( !str_cmp( buf, "endif" ) )
       return com_list;
     cmnd     = com_list;
     com_list = tprg_next_command( com_list );
     while ( *cmnd == ' ' )
       cmnd++;
     if ( *cmnd == '\0' )
       { 
       /*
	 bug ( "Mob: %d missing endif", mob->pIndexData->vnum );  */
	 return null;
       }
     morebuf = one_argument( cmnd, buf );
     
     for ( ; ; ) /*process the post-else commands until an endif is found.*/
       {
	 if ( !str_cmp( buf, "if" ) )
	   { 
	     com_list = tprg_process_if( morebuf, com_list, room, tObj, actor,
					 obj, vo, rndm );
	     while ( *cmnd == ' ' )
	       cmnd++;
	     if ( *com_list == '\0' )
	       return null;
	     cmnd     = com_list;
	     com_list = tprg_next_command( com_list );
	     morebuf  = one_argument( cmnd,buf );
	     continue;
	   }
	 if ( !str_cmp( buf, "else" ) ) 
	   {
	   /*
	     bug ( "Mob: %d found else in an else section",
		  mob->pIndexData->vnum );  */
	     return null;
	   }
	 if ( !str_cmp( buf, "break" ) )
	   return null;
	 if ( !str_cmp( buf, "endif" ) )
	   return com_list; 
	 tprg_process_cmnd( cmnd, room, tObj, actor, obj, vo, rndm );
	 cmnd     = com_list;
	 com_list = tprg_next_command( com_list );
	 while ( *cmnd == ' ' )
	   cmnd++;
	 if ( *cmnd == '\0' )
	   {
	   /*
	     bug ( "Mob:%d missing endif in else section",
		  mob->pIndexData->vnum );  */
	     return null;
	   }
	 morebuf = one_argument( cmnd, buf );
       }
   }
}

/* This routine handles the variables for command expansion.
 * If you want to add any go right ahead, it should be fairly
 * clear how it is done and they are quite easy to do, so you
 * can be as creative as you want. The only catch is to check
 * that your variables exist before you use them. At the moment,
 * using $t when the secondary target refers to an object 
 * i.e. >prog_act drops~<nl>if ispc($t)<nl>sigh<nl>endif<nl>~<nl>
 * probably makes the mud crash (vice versa as well) The cure
 * would be to change act() so that vo becomes vict & v_obj.
 * but this would require a lot of small changes all over the code.
 */
void tprg_translate( char ch, char *t, ROOM_INDEX_DATA *room,
		    OBJ_INDEX_DATA *tObj, CHAR_DATA *actor,
                    OBJ_DATA *obj, void *vo, CHAR_DATA *rndm )
{
 static char *he_she        [] = { "it",  "he",  "she" };
 static char *him_her       [] = { "it",  "him", "her" };
 static char *his_her       [] = { "its", "his", "her" };
 CHAR_DATA   *vict             = (CHAR_DATA *) vo;
 OBJ_DATA    *v_obj            = (OBJ_DATA  *) vo;

 *t = '\0';
 switch ( ch ) {
     case 'n':
         if ( actor )
	 {
	   if ( can_see( mob,actor ) )
	     one_argument( actor->name, t );
	   if ( !IS_NPC( actor ) )
	     *t = UPPER( *t );
	 }
	 else
	   strcpy( t, "someone" );
      break;

     case 'N':
         if ( actor ) 
            if ( can_see( mob, actor ) )
	       if ( IS_NPC( actor ) )
		 strcpy( t, actor->short_descr );
	       else
	       {
		   strcpy( t, actor->name );
		   strcat( t, actor->pcdata->title );
	       }
	    else
	      strcpy( t, "someone" );
	 break;

     case 't':
         if ( vict )
	 {
	   if ( can_see( mob, vict ) )
	     one_argument( vict->name, t );
	   if ( !IS_NPC( vict ) )
	     *t = UPPER( *t );
	 }
	 else
	   strcpy( t, "someone" );
	 break;

     case 'T':
         if ( vict ) 
            if ( can_see( mob, vict ) )
	       if ( IS_NPC( vict ) )
		 strcpy( t, vict->short_descr );
	       else
	       {
		 strcpy( t, vict->name );
		 strcat( t, " " );
		 strcat( t, vict->pcdata->title );
	       }
	    else
	      strcpy( t, "someone" );
	 break;
     
     case 'r':
         if ( rndm )
	 {
	   if ( can_see( mob, rndm ) )
	     one_argument( rndm->name, t );
	   if ( !IS_NPC( rndm ) )
	     *t = UPPER( *t );
	 }
	 else
	   strcpy(t, "someone" );
      break;

     case 'R':
         if ( rndm ) 
            if ( can_see( mob, rndm ) )
	       if ( IS_NPC( rndm ) )
		 strcpy(t,rndm->short_descr);
	       else
	       {
		 strcpy( t, rndm->name );
		 strcat( t, rndm->pcdata->title );
	       }
	    else
	      strcpy( t, "someone" );
	 break;

     case 'e':
         if ( actor )
	   can_see( mob, actor ) ? strcpy( t, he_she[ actor->sex ] )
	                         : strcpy( t, "someone" );
	 break;
  
     case 'm':
         if ( actor )
	   can_see( mob, actor ) ? strcpy( t, him_her[ actor->sex ] )
                                 : strcpy( t, "someone" );
	 break;
  
     case 's':
         if ( actor )
	   can_see( mob, actor ) ? strcpy( t, his_her[ actor->sex ] )
	                         : strcpy( t, "someone's" );
	 break;
     
     case 'E':
         if ( vict )
	   can_see( mob, vict ) ? strcpy( t, he_she[ vict->sex ] )
                                : strcpy( t, "someone" );
	 break;
  
     case 'M':
         if ( vict )
	   can_see( mob, vict ) ? strcpy( t, him_her[ vict->sex ] )
                                : strcpy( t, "someone" );
	 break;
  
     case 'S':
         if ( vict )
	   can_see( mob, vict ) ? strcpy( t, his_her[ vict->sex ] )
                                : strcpy( t, "someone's" ); 
	 break;

     case 'j':
	 strcpy( t, he_she[ mob->sex ] );
	 break;
  
     case 'k':
	 strcpy( t, him_her[ mob->sex ] );
	 break;
  
     case 'l':
	 strcpy( t, his_her[ mob->sex ] );
	 break;

     case 'J':
         if ( rndm )
	   can_see( mob, rndm ) ? strcpy( t, he_she[ rndm->sex ] )
	                        : strcpy( t, "someone" );
	 break;
  
     case 'K':
         if ( rndm )
	   can_see( mob, rndm ) ? strcpy( t, him_her[ rndm->sex ] )
                                : strcpy( t, "someone" );
	 break;
  
     case 'L':
         if ( rndm )
	   can_see( mob, rndm ) ? strcpy( t, his_her[ rndm->sex ] )
	                        : strcpy( t, "someone's" );
	 break;

     case 'o':
         if ( obj )
	   can_see_obj( mob, obj ) ? one_argument( obj->name, t )
                                   : strcpy( t, "something" );
	 break;

     case 'O':
         if ( obj )
	   can_see_obj( mob, obj ) ? strcpy( t, obj->short_descr )
                                   : strcpy( t, "something" );
	 break;

     case 'p':
         if ( v_obj )
	   can_see_obj( mob, v_obj ) ? one_argument( v_obj->name, t )
                                     : strcpy( t, "something" );
	 break;

     case 'P':
         if ( v_obj )
	   can_see_obj( mob, v_obj ) ? strcpy( t, v_obj->short_descr )
                                     : strcpy( t, "something" );
      break;

     case 'a':
         if ( obj ) 
          switch ( *( obj->name ) )
	  {
	    case 'a': case 'e': case 'i':
            case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
	 break;

     case 'A':
         if ( v_obj ) 
          switch ( *( v_obj->name ) )
	  {
            case 'a': case 'e': case 'i':
	    case 'o': case 'u': strcpy( t, "an" );
	      break;
            default: strcpy( t, "a" );
          }
	 break;

     case '$':
         strcpy( t, "$" );
	 break;

     default:
         bug( "Mob: %d bad $var", mob->pIndexData->vnum );
	 break;
       }

 return;

}

/* This procedure simply copies the cmnd to a buffer while expanding
 * any variables by calling the translate procedure.  The observant
 * code scrutinizer will notice that this is taken from act()
 */
void tprg_process_cmnd( char *cmnd, ROOM_INDEX_DATA *room,
		        OBJ_INDEX_DATA *tObj, CHAR_DATA *actor,
			OBJ_DATA *obj, void *vo, CHAR_DATA *rndm )
{
  char buf[ MAX_INPUT_LENGTH ];
  char tmp[ MAX_INPUT_LENGTH ];
  char *str;
  char *i;
  char *point;
  CHAR_DATA *mob;

  point   = buf;
  str     = cmnd;

  while ( *str != '\0' )
  {
    if ( *str != '$' )
    {
      *point++ = *str++;
      continue;
    }
    str++;
    tprg_translate( *str, tmp, room, tObj, actor, obj, vo, rndm );
    i = tmp;
    ++str;
    while ( ( *point = *i ) != '\0' )
      ++point, ++i;
  }
  *point = '\0';
  
  mob = create_mobile( MOB_VNUM_SUPERMOB );
  mob->player_name = str_dup( ( room ) ? room->name : tObj->name );
  mob->short_desc = str_dup( ( room ) ? room->name : tObj->name );
  mob->level = ( actor ) ? actor->level + 5 : 10;
  char_to_room( mob, ( room ) ? room : ( tObj ) ? ( tObj->carried_by ) ?
  tObj->carried_by->in_room : tObj->in_room );
  interpret( mob, buf );
  char_from_room( mob );
  extract_char( mob );
  return;
}

/* The main focus of the MOBprograms.  This routine is called 
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
void tprg_driver ( char *com_list,ROOM_INDEX_DATA *room,
		   OBJ_INDEX_DATA *tObj, CHAR_DATA *actor,
		   OBJ_DATA *obj, void *vo)
{

 char tmpcmndlst[ MAX_STRING_LENGTH ];
 char buf       [ MAX_INPUT_LENGTH ];
 char *morebuf;
 char *command_list;
 char *cmnd;
 CHAR_DATA *rndm  = NULL;
 CHAR_DATA *vch   = NULL;
 int        count = 0;

/*
 * For things like rand_progs with $r in them, ect.. that really have no
 * effect on the game, but make a lotta bugs if they have stuff like
 * 'MPKill $r'..
 * And for those people who have things like rand_progs with chats...
 * They're too annoying anyways.. :)..
 * -- Altrag
 */

 if ( room )
   if ( !room->people )
     return;
 
 if ( tObj )
   if ( tObj->in_room && !tObj->in_room->people )
     return;
     

 if ( actor && !IS_NPC(actor) && IS_SET(actor->act, PLR_WIZINVIS) &&
      actor->wizinvis > LEVEL_MORTAL )
   return;

 /* get a random visable mortal player who is in the room with the mob */
 if ( room )
   for ( vch = room->people; vch; vch = vch->next_in_room )
     {
       if ( number_range( 0, count ) == 0 )
         rndm = vch;
       count++;
     }

 if ( tObj )
   if ( !tObj->carried_by )
   {
      for ( vch = tObj->in_room->people; vch; vch = vch->next_in_room )
      {
       if ( number_range( 0, count ) == 0 )
        rndm = vch;
       count++;
      }
   }
   else
     for ( vch = tObj->carried_by->in_room->people; vch; vch = vch->next_in_room )
       {
         if ( number_range( 0, count ) == 0 )
           rndm = vch;
         count++;
       }
 
 strcpy( tmpcmndlst, com_list );
 command_list = tmpcmndlst;
 cmnd         = command_list;
 command_list = tprg_next_command( command_list );
 while ( *cmnd != '\0' )
 {
     morebuf = one_argument( cmnd, buf );
     if ( !str_cmp( buf, "if" ) )
       command_list = tprg_process_if( morebuf, command_list, room, tObj,
				       actor, obj, vo, rndm );
     else
       tprg_process_cmnd( cmnd, room, tObj, actor, obj, vo, rndm );
     cmnd         = command_list;
     command_list = tprg_next_command( command_list );
 }

 return;
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
void tprg_wordlist_check( char *arg, ROOM_INDEX_DATA *room,
			  OBJ_INDEX_DATA *tObj, CHAR_DATA *actor,
			  OBJ_DATA *obj, void *vo, int type )
{

  char        temp1[ MAX_STRING_LENGTH ];
  char        temp2[ MAX_INPUT_LENGTH ];
  char        word[ MAX_INPUT_LENGTH ];
  TRAP_DATA  *tprg;
  char       *list;
  char       *start;
  char       *dupl;
  char       *end;
  int         i;

  for ( tprg = ( ( room ) ? room->traps : tObj->traps ); tprg != NULL; tprg = tprg->next )
    if ( tprg->type & type )
      {
	strcpy( temp1, tprg->arglist );
	list = temp1;
	for ( i = 0; i < strlen( list ); i++ )
	  list[i] = LOWER( list[i] );
	strcpy( temp2, arg );
	dupl = temp2;
	for ( i = 0; i < strlen( dupl ); i++ )
	  dupl[i] = LOWER( dupl[i] );
	if ( ( list[0] == 'p' ) && ( list[1] == ' ' ) )
	  {
	    list += 2;
	    while ( ( start = strstr( dupl, list ) ) )
	      if ( (start == dupl || *(start-1) == ' ' )
		  && ( *(end = start + strlen( list ) ) == ' '
		      || *end == '\n'
		      || *end == '\r'
		      || *end == '\0' ) )
		{
		  tprg_driver( tprg->comlist, room, tObj, actor, obj, vo );
		  break;
		}
	      else
		dupl = start+1;
	  }
	else
	  {
	    list = one_argument( list, word );
	    for( ; word[0] != '\0'; list = one_argument( list, word ) )
	      while ( ( start = strstr( dupl, word ) ) )
		if ( ( start == dupl || *(start-1) == ' ' )
		    && ( *(end = start + strlen( word ) ) == ' '
			|| *end == '\n'
			|| *end == '\r'
			|| *end == '\0' ) )
		  {
		    tprg_driver( tprg->comlist, room, tObj, actor, obj, vo );
		    break;
		  }
		else
		  dupl = start+1;
	  }
      }

  return;

}

void tprg_percent_check( ROOM_INDEX_DATA *room, OBJ_INDEX_DATA *tObj,
 			 CHAR_DATA *actor, OBJ_DATA *obj,
			 void *vo, int type)
{
 TRAP_DATA * tprg;

 for ( tprg =( ( room ) ? room->traps : tObj->traps ); tprg != NULL; tprg = tprg->next )
   if ( ( tprg->type & type )
       && ( number_percent( ) < atoi( trpg->arglist ) ) )
     {
       tprg_driver( mprg->comlist, room, tObj, actor, obj, vo );
     }

 return;

}

/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the tprg_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void tprg_act_trigger( char *buf, CHAR_DATA *mob, CHAR_DATA *ch,
		       OBJ_DATA *obj, void *vo)
{
  MPROG_ACT_LIST * tmp_act;

  if ( IS_NPC( mob )
      && ( mob->pIndexData->progtypes & ACT_PROG ) )
    {
      tmp_act = alloc_mem( sizeof( MPROG_ACT_LIST ) , "t_prog.c 1672");
/*      if ( mob->mpactnum > 0 )
	tmp_act->next = mob->mpact->next;
      else
	tmp_act->next = NULL;*/
      tmp_act->next = mob->mpact;

      mob->mpact      = tmp_act;
      mob->mpact->buf = str_dup( buf );
      mob->mpact->ch  = ch; 
      mob->mpact->obj = obj; 
      mob->mpact->vo  = vo; 
      mob->mpactnum++;
    }
/*  if ( IS_NPC( mob ) && ( mob->pIndexData->progtypes & ACT_PROG )
       && mob->in_room->area->nplayer > 0 )
    tprg_wordlist_check( buf, mob, ch, obj, vo, ACT_PROG );*/

  return;

}

void tprg_bribe_trigger( CHAR_DATA *mob, CHAR_DATA *ch, MONEY_DATA *amount )
{
  char        buf[ MAX_STRING_LENGTH ];
  MPROG_DATA *mprg;
  OBJ_DATA   *obj;

  if ( IS_NPC( mob )
      && ( mob->pIndexData->progtypes & BRIBE_PROG ) )
    {
      obj = create_object( get_obj_index( OBJ_VNUM_MONEY_SOME ), 0 );
      obj->value[0] = amount.gold;
      obj->value[1] = amount.silver;
      obj->value[2] = amount.copper;
      obj_to_char( obj, mob );
      sub_money( &mob->money, amount );

      for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
        if ( ( mprg->type & BRIBE_PROG )
            && ( (amount.gold + (amount.silver/SILVER_PER_GOlD) +
		 (amount.copper/COPPER_PER_GOLD) ) >= atoi( mprg->arglist ) ) )
          {
            tprg_driver( mprg->comlist, mob, ch, obj, NULL );
            break;
          }
    }
  
  return;

}

void tprg_death_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & DEATH_PROG ) )
   {
     int oldpos;

     oldpos = mob->position;
     mob->position = POS_STANDING;
     tprg_percent_check( mob, ch, NULL, NULL, DEATH_PROG );
     mob->position = oldpos;
   }

 death_cry( mob );
 return;

}

void tprg_entry_trigger( CHAR_DATA *mob )
{

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & ENTRY_PROG ) )
   tprg_percent_check( mob, NULL, NULL, NULL, ENTRY_PROG );

 return;

}

void tprg_fight_trigger( CHAR_DATA *mob, CHAR_DATA *ch )
{
 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & FIGHT_PROG ) )
 {
   tprg_percent_check( mob, ch, NULL, NULL, FIGHT_PROG );
 }
 return;
}

void tprg_give_trigger( CHAR_DATA *mob, CHAR_DATA *ch, OBJ_DATA *obj )
{

 char        buf[MAX_INPUT_LENGTH];
 MPROG_DATA *mprg;

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & GIVE_PROG ) )
 {
   for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
     {
       one_argument( mprg->arglist, buf );
       if ( ( mprg->type & GIVE_PROG )
	   && ( ( !str_cmp( obj->name, mprg->arglist ) )
	       || ( !str_cmp( "all", buf ) ) ) )
	 {
	   tprg_driver( mprg->comlist, mob, ch, obj, NULL );
	   break;
	 }
     }
 }

 return;

}

void tprg_greet_trigger( CHAR_DATA *ch )
{

 CHAR_DATA *vmob;

 for ( vmob = ch->in_room->people; vmob != NULL; vmob = vmob->next_in_room )
   if ( IS_NPC( vmob )
       && ch != vmob
       && can_see( vmob, ch )
       && ( vmob->fighting == NULL )
       && IS_AWAKE( vmob )
       && ( vmob->pIndexData->progtypes & GREET_PROG) )
   {
     tprg_percent_check( vmob, ch, NULL, NULL, GREET_PROG );
   }

 return;

}

void tprg_hitprcnt_trigger( CHAR_DATA *mob, CHAR_DATA *ch)
{

 MPROG_DATA *mprg;

 if ( IS_NPC( mob )
     && ( mob->pIndexData->progtypes & HITPRCNT_PROG ) )
 {
   for ( mprg = mob->pIndexData->mobprogs; mprg != NULL; mprg = mprg->next )
     if ( ( mprg->type & HITPRCNT_PROG )
	 && ( ( 100*mob->hit / mob->max_hit ) < atoi( mprg->arglist ) ) )
       {
	 tprg_driver( mprg->comlist, mob, ch, NULL, NULL );
	 break;
       }
 }
 
 return;

}

void tprg_random_trigger( CHAR_DATA *mob )
{

  if ( mob->pIndexData->progtypes & RAND_PROG)
    tprg_percent_check(mob,NULL,NULL,NULL,RAND_PROG);

  return;

}

void tprg_speech_trigger( char *txt, CHAR_DATA *mob )
{

  CHAR_DATA *vmob;

  for ( vmob = mob->in_room->people; vmob != NULL; vmob = vmob->next_in_room )
    if ( IS_NPC( vmob ) && ( vmob->pIndexData->progtypes & SPEECH_PROG ) )
    {
      tprg_wordlist_check( txt, vmob, mob, NULL, NULL, SPEECH_PROG );
    }
  
  return;

}
