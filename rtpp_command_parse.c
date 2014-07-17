/*
 * Copyright (c) 2004-2006 Maxim Sobolev <sobomax@FreeBSD.org>
 * Copyright (c) 2006-2014 Sippy Software, Inc., http://www.sippysoft.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>

#include "rtpp_log.h"
#include "rtpp_defines.h"
#include "rtpp_command.h"
#include "rtpp_command_parse.h"
#include "rtpp_command_private.h"

struct cmd_props {
    int max_argc;
    int min_argc;
    int has_cmods;
    int has_call_id;
    int fpos;
    int tpos;
};

static int 
fill_cmd_props(struct rtpp_command *cmd, struct common_cmd_args *cca,
  struct cmd_props *cpp)
{

    cpp->has_call_id = 1;
    cpp->fpos = -1;
    cpp->tpos = -1;
    switch (cmd->argv[0][0]) {
    case 'u':
    case 'U':
        cca->op = UPDATE;
        cca->rname = "update/create";
        cca->hint = "U[opts] callid remote_ip remote_port from_tag [to_tag] [notify_socket notify_tag]";
        cpp->max_argc = 8;
        cpp->min_argc = 5;
        cpp->has_cmods = 1;
        cpp->fpos = 4;
        cpp->tpos = 5;
        break;

    case 'l':
    case 'L':
        cca->op = LOOKUP;
        cca->rname = "lookup";
        cca->hint = "L[opts] callid remote_ip remote_port from_tag [to_tag]";
        cpp->max_argc = 6;
        cpp->min_argc = 5;
        cpp->has_cmods = 1;
        cpp->fpos = 4;
        cpp->tpos = 5;
        break;

    case 'd':
    case 'D':
        cca->op = DELETE;
        cca->rname = "delete";
        cca->hint = "D[w] callid from_tag [to_tag]";
        cpp->max_argc = 4;
        cpp->min_argc = 3;
        cpp->has_cmods = 1;
        cpp->fpos = 2;
        cpp->tpos = 3;
        break;

    case 'p':
    case 'P':
        cca->op = PLAY;
        cca->rname = "play";
        cca->hint = "P[n] callid pname codecs from_tag [to_tag]";
        cpp->max_argc = 6;
        cpp->min_argc = 5;
        cpp->has_cmods = 1;
        cpp->fpos = 4;
        cpp->tpos = 5;
        break;

    case 'r':
    case 'R':
        cca->op = RECORD;
        cca->rname = "record";
        cca->hint = "R[-xxx-] call_id from_tag [to_tag]";
        cpp->max_argc = 4;
        cpp->min_argc = 3;
        cpp->has_cmods = 1;
        cpp->fpos = 2;
        cpp->tpos = 3;
        break;

    case 'c':
    case 'C':
        cca->op = COPY;
        cca->rname = "copy";
        cca->hint = "C[-xxx-] call_id -XXX- from_tag [to_tag]";
        cpp->max_argc = 5;
        cpp->min_argc = 4;
        cpp->has_cmods = 1;
        cpp->fpos = 3;
        cpp->tpos = 4;
        break;

    case 's':
    case 'S':
        cca->op = NOPLAY;
        cca->rname = "noplay";
        cca->hint = "S call_id from_tag [to_tag]";
        cpp->max_argc = 4;
        cpp->min_argc = 3;
        cpp->has_cmods = 0;
        cpp->fpos = 2;
        cpp->tpos = 3;
        break;

    case 'v':
    case 'V':
        if (cmd->argv[0][1] == 'F' || cmd->argv[0][1] == 'f') {
            cca->op = VER_FEATURE;
            cca->rname = "feature_check";
            cca->hint = "VF feature_num";
            cpp->max_argc = 2;
            cpp->min_argc = 2;
            cpp->has_cmods = 1;
            cpp->has_call_id = 0;
            break;
        }
        cca->op = VERSION;
        cca->rname = "get_version";
        cca->hint = "V";
        cpp->max_argc = 1;
        cpp->min_argc = 1;
        cpp->has_cmods = 0;
        cpp->has_call_id = 0;
        break;

    case 'i':
    case 'I':
        cca->op = INFO;
        cca->rname = "get_info";
        cca->hint = "I[b]";
        cpp->max_argc = 1;
        cpp->min_argc = 1;
        cpp->has_cmods = 1;
        cpp->has_call_id = 0;
        break;

    case 'q':
    case 'Q':
        cca->op = QUERY;
        cca->rname = "query";
        cca->hint = "Q call_id from_tag [to_tag]";
        cpp->max_argc = 4;
        cpp->min_argc = 3;
        cpp->has_cmods = 0;
        cpp->fpos = 2;
        cpp->tpos = 3;
        break;

    case 'x':
    case 'X':
        cca->op = DELETE_ALL;
        cca->rname = "delete_all";
        cca->hint = "X";
        cpp->max_argc = 1;
        cpp->min_argc = 1;
        cpp->has_cmods = 0;
        cpp->has_call_id = 0;
        break;

    default:
        return (-1);
    }
    return (0);
}

int
rtpp_command_pre_parse(struct cfg *cf, struct rtpp_command *cmd,
  struct common_cmd_args *cca)
{
    struct cmd_props cprops;

    if (fill_cmd_props(cmd, cca, &cprops) != 0) {
        rtpp_log_write(RTPP_LOG_ERR, cf->stable.glog, "unknown command \"%c\"",
          cmd->argv[0][0]);
        reply_error(cf, cmd, ECODE_CMDUNKN);
        return (-1);
    }
    if (cmd->argc < cprops.min_argc || cmd->argc > cprops.max_argc) {
        rtpp_log_write(RTPP_LOG_ERR, cf->stable.glog, "%s command syntax error"
          ": invalid number of arguments (%d)", cca->rname, cmd->argc);
        reply_error(cf, cmd, ECODE_PARSE_NARGS);
        return (-1);
    }
    if (cprops.has_cmods == 0 && cmd->argv[0][1] != '\0') {
        rtpp_log_write(RTPP_LOG_ERR, cf->stable.glog, "%s command syntax error"
          ": modifiers are not supported by the command", cca->rname);
        reply_error(cf, cmd, ECODE_PARSE_MODS);
        return (-1);
    }
    cca->call_id = cprops.has_call_id ? cmd->argv[1] : NULL;
    cca->from_tag = cprops.fpos > 0 ? cmd->argv[cprops.fpos] : NULL;
    cca->to_tag = cprops.tpos > 0 ? cmd->argv[cprops.tpos] : NULL;
    return (0);
}