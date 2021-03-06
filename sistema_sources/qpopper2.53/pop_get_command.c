/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1997 by Qualcomm Incorporated.
 */


#include <config.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include "popper.h"

/* 
 *  get_command:    Extract the command from an input line form a POP client
 */

static state_table states[] = {
        auth1,  "user", 1,  1,  pop_user,   {auth1, auth2},
        auth2,  "pass", 1,  1,  pop_pass,   {halt,  trans},
        auth1,  "auth", 1,  2,  pop_auth,   {auth1, auth1},
#ifdef RPOP
        auth2,  "rpop", 1,  1,  pop_rpop,   {halt,  trans},
#endif
#ifdef APOP
        auth1,  "apop", 2,  2,  pop_apop,   {halt,  trans},
#endif
        auth1,  "quit", 0,  0,  pop_quit,   {halt,  halt},
        auth2,  "quit", 0,  0,  pop_quit,   {halt,  halt},
        trans,  "stat", 0,  0,  pop_stat,   {trans, trans},
        trans,  "list", 0,  1,  pop_list,   {trans, trans},
        trans,  "retr", 1,  1,  pop_send,   {trans, trans},
        trans,  "dele", 1,  1,  pop_dele,   {trans, trans},
        trans,  "noop", 0,  0,  NULL,       {trans, trans},
        trans,  "rset", 0,  0,  pop_rset,   {trans, trans},
        trans,  "top",  2,  2,  pop_send,   {trans, trans},
        trans,  "last", 0,  0,  pop_last,   {trans, trans},
        trans,  "xtnd", 1,  99, pop_xtnd,   {trans, trans},
        trans,  "uidl", 0,  1,  pop_uidl,   {trans, trans},
        trans,  "euidl",0,  1,  pop_euidl,  {trans, trans},
        trans,  "quit", 0,  0,  pop_updt,   {halt,  halt},
        (state) 0,  NULL,   0,  0,  NULL,       {halt,  halt},
};

state_table *pop_get_command(p,mp)
POP             *   p;
register char   *   mp;         /*  Pointer to unparsed line 
                                    received from the client */
{
    state_table     *   s;
    char                buf[MAXMSGLINELEN];

    /*  Save a copy of the original client line */
#ifdef DEBUG
    if(p->debug) strncpy(buf, mp, sizeof(buf));
#endif

    /*  Parse the message into the parameter array */
    if ((p->parm_count = pop_parse(p,mp)) < 0) return(NULL);

    /*  Do not log cleartext passwords */
#ifdef DEBUG
    if(p->debug){
        if(strcmp(p->pop_command,"pass") == 0)
            pop_log(p,POP_DEBUG,"Received: \"%.128s xxxxxxxxx\"",p->pop_command);
        else {
            /*  Remove trailing <LF> */
            buf[strlen(buf)-2] = '\0';
            pop_log(p,POP_DEBUG,"Received: \"%.128s\"",buf);
        }
    }
#endif

    /*  Search for the POP command in the command/state table */
    for (s = states; s->command; s++) {

        /*  Is this a valid command for the current operating state? */
        if (strcmp(s->command,p->pop_command) == 0
             && s->ValidCurrentState == p->CurrentState) {

            /*  Were too few parameters passed to the command? */
            if (p->parm_count < s->min_parms) {
                pop_msg(p,POP_FAILURE,
                    "Too few arguments for the %.128s command.",p->pop_command);
                return((state_table *)0);
            }
            

            /*  Were too many parameters passed to the command? */
            if (p->parm_count > s->max_parms) {
                pop_msg(p,POP_FAILURE,
                    "Too many arguments for the %.128s command.",p->pop_command);
                return((state_table *)0);
            }
            

            /*  Return a pointer to the entry for this command in 
                the command/state table */
            return (s);
        }
    }
    /*  The client command was not located in the command/state table */
    pop_msg(p,POP_FAILURE,"Unknown command: \"%.128s\".",p->pop_command);
    return((state_table *)0);
}
