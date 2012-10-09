/*
 *   main.c
 *
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2005
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 */

// Standard Linux headers
#include     <stdio.h>              // Always include this header
#include     <stdlib.h>             // Always include this header
#include     <signal.h>             // Defines signal-handling functions (i.e. trap Ctrl-C)
#include     <unistd.h>
#include     <pthread.h>

// Application headers
#include     "debug.h"
#include     "video_thread.h"
#include     "audio_thread.h"
#include     "thread.h"

/* Global thread environments */
video_thread_env video_env = {0};
audio_thread_env audio_env = {0};

// MP3 while loop
int inputloop = 1;

/* Store previous signal handler and call it */
void (*pSigPrev)(int sig);

/* Callback called when SIGINT is sent to the process (Ctrl-C) */
void signal_handler(int sig)
{
    DBG( "Ctrl-C pressed, cleaning up and exiting..\n" );
	inputloop = 0;
    video_env.quit = 1;

    #ifdef _DEBUG_
	sleep(1);
    #endif

    audio_env.quit = 1;

    if( pSigPrev != NULL )
        (*pSigPrev)( sig );
}

//*****************************************************************************
//*  main
//*****************************************************************************
int main(int argc, char *argv[])
{
   pthread_t audioThread, videoThread;
   void *audioThreadReturn, *videoThreadReturn;

/* The levels of initialization for initMask */
#define VIDEOTHREADATTRSCREATED 0x1
#define VIDEOTHREADCREATED      0x2
#define AUDIOTHREADCREATED      0x4
    unsigned int    initMask  = 0;
    int             status    = EXIT_SUCCESS;

    /* Set the signal callback for Ctrl-C */
    pSigPrev = signal( SIGINT, signal_handler );

    system("cd ..; ./vid2Show");

    /* Create a thread for video */
    DBG( "Creating video thread\n" );
    printf( "\tPress Ctrl-C to exit\n" );

    if(launch_pthread(&videoThread, TIMESLICE, 0, video_thread_fxn, (void *) &video_env) != thread_SUCCESS){
	ERR("pthread create failed for video thread\n");
	status = EXIT_FAILURE;
	goto cleanup;
    }

    initMask |= VIDEOTHREADCREATED;

    #ifdef _DEBUG_
	sleep(1);
    #endif

    if(launch_pthread(&audioThread, REALTIME, 99, audio_thread_fxn, (void *) &audio_env) != thread_SUCCESS){
	ERR("pthread create failed for video thread\n");
	status = EXIT_FAILURE;
	goto cleanup;
    }

    initMask |= AUDIOTHREADCREATED;

// Keeps checking for user input
    while(inputloop){
	char userInput = getchar();
	if (userInput == '\n'){
		audio_env.replay = 1;
    		video_env.pause = 1;
    		printf("Enter key pressed!\n");
	}
    }
	
    pthread_join(videoThread, (void **) &videoThreadReturn);
    pthread_join(audioThread, (void **) &audioThreadReturn);

  cleanup:
    system("cd ..; ./resetVideo");

    exit( status );
}
