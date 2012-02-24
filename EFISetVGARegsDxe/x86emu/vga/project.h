/*
 * project.h:
 * All definitions and includes for mbefi.
 *
 * $Id: project.h,v 1.2 2006/05/28 14:51:20 root Exp $
 */

#ifndef __PROJECT_H__
#define __PROJECT_H__

#include <efi.h>
#include <efiapi.h>
#include <efilib.h>
#include <efifs.h>
#include <efigpt.h>
#include <efinet.h>

#define MAX_PATH_LEN 1024
#define TFTP_BLOCK_SIZE 512
#define ELILO_BUFFER_LEN (2*1024*1024)
#define MAX_VARIABLE_LEN 2048
#define NETLOG_MAXLEN  1500
#define LOG_PRINT_LEN 4096
#define MAC_LEN   6
#define MAXPARTS 255

#define CONFIG_IA32 1
#define NO_LONG_LONG 1
#define IN_MODULE 1

typedef unsigned int size_t;


#define LogPrint(args...)  do { SPrint(log_print_buf, sizeof(log_print_buf), args ); do_log_print(log_print_buf); } while (0)

#define DEBUG_POINT do { LogPrint(L"DEBUG at %a:%d\n",__FILE__,__LINE__); } while (0)

#include "pci.h"
#include "vbetool/vbetool.h"

#include "roms.h"
#include "prototypes.h"

#define print(a...) LogPrint( L ## a )

extern uint8_t *real_memory;

#endif /* __PROJECT_H__ */
