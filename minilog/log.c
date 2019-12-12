/*
 * Copyright (C), 2000-2007 by the monit project group.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <config.h>

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include "monitor.h"


/**
 *  Implementation of a logger that appends log messages to a file
 *  with a preceding timestamp. Methods support both syslog or own
 *  logfile.
 *
 *  @author Jan-Henrik Haukeland, <hauk@tildeslash.com>
 *
 *  @version \$Id: log.c,v 1.31 2007/01/03 09:31:01 martinp Exp $
 *
 *  @file
 */


/* ------------------------------------------------------------- Definitions */


static FILE *LOG= NULL;
static pthread_mutex_t log_mutex= PTHREAD_MUTEX_INITIALIZER;


static struct mylogpriority {
  int  priority;
  char *description;
} logPriority[]= {
  {LOG_EMERG,   "emergency"},
  {LOG_ALERT,   "alert"},
  {LOG_CRIT,    "critical"},
  {LOG_ERR,     "error"},
  {LOG_WARNING, "warning"},
  {LOG_NOTICE,  "notice"},
  {LOG_INFO,    "info"},
  {LOG_DEBUG,   "debug"},
  {-1,          NULL}
};


/* -------------------------------------------------------------- Prototypes */


static int  open_log();
static char *timefmt(char *t, int size);
static const char *logPriorityDescription(int p);
static void log_log(int priority, const char *s, va_list ap);


/* ------------------------------------------------------------------ Public */


/**
 * Initialize the log system and 'log' function
 * @return TRUE if the log system was successfully initialized
 */
int log_init() {

  if(!Run.dolog) {
    return TRUE;
  }

  if(!open_log()) {
    return FALSE;
  }

  /* Register log_close to be
     called at program termination */
  atexit(log_close);

  return TRUE;
  
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogEmergency(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_EMERG, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogAlert(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_ALERT, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogCritical(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_CRIT, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogError(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_ERR, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogWarning(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_WARNING, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogNotice(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_NOTICE, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogInfo(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_INFO, s, ap);
  va_end(ap);
}


/**
 * Logging interface with priority support
 * @param s A formated (printf-style) string to log
 */
void LogDebug(const char *s, ...) {
  va_list ap;

  ASSERT(s);

  va_start(ap, s);
  log_log(LOG_DEBUG, s, ap);
  va_end(ap);
}


/**
 * Close the log file or syslog
 */
void log_close() {
  
  if(Run.use_syslog) {
    closelog(); 
  }
  
  if(LOG  && (0 != fclose(LOG))) {
    LogError("%s: Error closing the log file -- %s\n",
	prog, STRERROR);
  }
  pthread_mutex_destroy(&log_mutex);
  LOG= NULL;
  
}


/* ----------------------------------------------------------------- Private */


/**
 * Open a log file or syslog
 */
static int open_log() {
 
  if(Run.use_syslog) {
    openlog(prog, LOG_PID, Run.facility); 
  } else {
    umask(LOGMASK);
    if((LOG= fopen(Run.logfile,"a+")) == (FILE *)NULL) {
      LogError("%s: Error opening the log file '%s' for writing -- %s\n",
	  prog, Run.logfile, STRERROR);
      return(FALSE);
    }
    /* Set logger in unbuffered mode */
    setvbuf(LOG, NULL, _IONBF, 0);
  }

  return TRUE;
  
}


/**
 * Returns the current time as a formated string, see the TIMEFORMAT
 * macro in monitor.h
 */
static char *timefmt(char *t, int size) {

  time_t now;
  struct tm tm;
  
  time(&now);
  if(!strftime(t, size, TIMEFORMAT, localtime_r(&now, &tm))) {
    *t= 0;
  }
  
  return t;
  
}


/**
 * Get a textual description of the actual log priority.
 * @param p The log priority
 * @return A string describing the log priority in clear text. If the
 * priority is not found NULL is returned.
 */
static const char *logPriorityDescription(int p) {

  struct mylogpriority *lp= logPriority;

  while((*lp).description)
  {
    if(p == (*lp).priority)
    {
      return (*lp).description;
    }
    lp++;
  }

  return NULL;

}


/**
 * Log a message to monits logfile or syslog. 
 * @param priority A message priority
 * @param s A formated (printf-style) string to log
 */
static void log_log(int priority, const char *s, va_list ap) {

#ifdef HAVE_VA_COPY
  va_list ap_copy;
#endif

  ASSERT(s);
  
  LOCK(log_mutex)
#ifdef HAVE_VA_COPY
    va_copy(ap_copy, ap);
    vfprintf(stderr, s, ap_copy);
    va_end(ap_copy);
#else
    vfprintf(stderr, s, ap);
#endif
    fflush(stderr);
  END_LOCK;
  
  if(Run.dolog) {
    char datetime[STRLEN];

    if(Run.use_syslog) {
      LOCK(log_mutex)
#ifdef HAVE_VA_COPY
        va_copy(ap_copy, ap);
        vsyslog(priority, s, ap_copy);
        va_end(ap_copy);
#else
        vsyslog(priority, s, ap);
#endif
      END_LOCK;
    } else if(LOG) {
      LOCK(log_mutex)
        fprintf(LOG, "[%s] %-8s : ",
          timefmt(datetime, STRLEN),
          logPriorityDescription(priority));
#ifdef HAVE_VA_COPY
        va_copy(ap_copy, ap);
      	vfprintf(LOG, s, ap_copy);
        va_end(ap_copy);
#else
        vfprintf(LOG, s, ap);
#endif
      END_LOCK;

    }
  }
}

