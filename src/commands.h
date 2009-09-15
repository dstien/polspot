/*
 * $Id: commands.h 326 2009-05-29 23:32:36Z dstien $
 *
 */

#ifndef DESPOTIFY_COMMANDS_H
#define DESPOTIFY_COMMANDS_H

void cmd_cb_connect();
void cmd_cb_disconnect();
void cmd_cb_search();
void cmd_cb_stop();
void cmd_cb_pause();
void cmd_cb_log();
void cmd_cb_help();
void cmd_cb_main();
void cmd_cb_redraw();
void cmd_cb_quit();

void command_process(char *str);

#endif
