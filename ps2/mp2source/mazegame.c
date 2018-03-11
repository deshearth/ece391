/*									tab:8
 *
 * mazegame.c - main source file for ECE398SSL maze game (F04 MP2)
 *
 * "Copyright (c) 2004-2009 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    4
 * Creation Date:   Fri Sep 10 09:57:54 2004
 * Filename:	    mazegame.c
 * History:
 *	SL	1	Fri Sep 10 09:57:54 2004
 *		First written.
 *	SL	2	Sat Sep 12 15:10:08 2009
 *		Integrated original release back into main code base.
 *	SL	3	Sun Sep 13 04:13:27 2009
 *		Replaced fruit display with Tux controller time display.
 *	SL	4	Sun Sep 13 12:46:13 2009
 *		Integrated cleanup mechanism to avoid problems with recovery.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "assert.h"
#include "blocks.h"
#include "input.h"
#include "maze.h"
#include "modex.h"
#include "text.h"


/*
 * If NDEBUG is not defined, we execute sanity checks to make sure that
 * changes to enumerations, bit maps, etc., have been made consistently.
 */
#if defined(NDEBUG)
#define sanity_check() 0
#else
static int sanity_check ();
#endif


/* a few constants */
#define PAN_BORDER      5  /* pan when border in maze squares reaches 5    */
#define MAX_LEVEL      10  /* maximum level number                         */

/* outcome of each level, and of the game as a whole */
typedef enum {GAME_WON, GAME_LOST, GAME_QUIT} game_condition_t;


/* structure used to hold game information */
typedef struct {
    /* parameters varying by level   */
    int number;                  /* starts at 1...                   */
    int maze_x_dim, maze_y_dim;  /* min to max, in steps of 2        */
    int initial_fruit_count;     /* 1 to 6, in steps of 1/2          */
    int time_to_first_fruit;     /* 300 to 120, in steps of -30      */
    int time_between_fruits;     /* 300 to 60, in steps of -30       */
    int tick_usec;		 /* 20000 to 5000, in steps of -1750 */
    
    /* dynamic values within a level -- you may want to add more... */
    unsigned int map_x, map_y;   /* current upper left display pixel */
} game_info_t;

static game_info_t game_info;


/* local functions--see function headers for details */
static int prepare_maze_level (int level);
static void move_up (int* ypos);
static void move_right (int* xpos);
static void move_down (int* ypos);
static void move_left (int* xpos);
static int unveil_around_player (int play_x, int play_y);
static int time_is_after (struct timeval* t1, struct timeval* t2);
static game_condition_t game_loop ();


/* 
 * prepare_maze_level
 *   DESCRIPTION: Prepare for a maze of a given level.  Fills the game_info
 *		  structure, creates a maze, and initializes the display.
 *   INPUTS: level -- level to be used for selecting parameter values
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, -1 on failure
 *   SIDE EFFECTS: writes entire game_info structure; changes maze;
 *                 initializes display
 */
static int
prepare_maze_level (int level)
{
    int i; /* loop index for drawing display */
    
    /*
     * Record level in game_info; other calculations use offset from
     * level 1.
     */
    game_info.number = level--;

    /* Set per-level parameter values. */
    if ((game_info.maze_x_dim = MAZE_MIN_X_DIM + 2 * level) >
	MAZE_MAX_X_DIM)
	game_info.maze_x_dim = MAZE_MAX_X_DIM;
    if ((game_info.maze_y_dim = MAZE_MIN_Y_DIM + 2 * level) >
	MAZE_MAX_Y_DIM)
	game_info.maze_y_dim = MAZE_MAX_Y_DIM;
    if ((game_info.initial_fruit_count = 1 + level / 2) > 6)
	game_info.initial_fruit_count = 6;
    if ((game_info.time_to_first_fruit = 300 - 30 * level) < 120)
	game_info.time_to_first_fruit = 120;
    if ((game_info.time_between_fruits = 300 - 60 * level) < 60)
	game_info.time_between_fruits = 60;
    if ((game_info.tick_usec = 20000 - 1750 * level) < 5000)
	game_info.tick_usec = 5000;

    /* Initialize dynamic values. */
    game_info.map_x = game_info.map_y = SHOW_MIN;

    /* Create a maze. */
    if (make_maze (game_info.maze_x_dim, game_info.maze_y_dim,
		   game_info.initial_fruit_count) != 0) {
	return -1;
    }
    
    /* Set logical view and draw initial screen. */
    set_view_window (game_info.map_x, game_info.map_y);
    for (i = 0; i < SCROLL_Y_DIM; i++) {
	(void)draw_horiz_line (i);
    }
    display_time_on_tux (0);
    
    /* Return success. */
    return 0;
}


/* 
 * move_up
 *   DESCRIPTION: Move the player up one pixel (assumed to be a legal move)
 *   INPUTS: ypos -- pointer to player's y position (pixel) in the maze
 *   OUTPUTS: *ypos -- reduced by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void
move_up (int* ypos)
{
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the upper pan border
     * while the top pixels of the maze are not on-screen.
     */
    if (--(*ypos) < game_info.map_y + BLOCK_Y_DIM * PAN_BORDER && 
	game_info.map_y > SHOW_MIN) {
	/*
	 * Shift the logical view upwards by one pixel and draw the
	 * new line.
	 */
	set_view_window (game_info.map_x, --game_info.map_y);
	(void)draw_horiz_line (0);
    }
}


/* 
 * move_right
 *   DESCRIPTION: Move the player right one pixel (assumed to be a legal move)
 *   INPUTS: xpos -- pointer to player's x position (pixel) in the maze
 *   OUTPUTS: *xpos -- increased by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void
move_right (int* xpos)
{
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the right pan border
     * while the rightmost pixels of the maze are not on-screen.
     */
    if (++(*xpos) > game_info.map_x + SCROLL_X_DIM -
	    BLOCK_X_DIM * (PAN_BORDER + 1) &&
	game_info.map_x + SCROLL_X_DIM < 
	    (2 * game_info.maze_x_dim + 1) * BLOCK_X_DIM - SHOW_MIN) {
	/*
	 * Shift the logical view to the right by one pixel and draw the
	 * new line.
	 */
	set_view_window (++game_info.map_x, game_info.map_y);
	(void)draw_vert_line (SCROLL_X_DIM - 1);
    }
}


/* 
 * move_down
 *   DESCRIPTION: Move the player right one pixel (assumed to be a legal move)
 *   INPUTS: ypos -- pointer to player's y position (pixel) in the maze
 *   OUTPUTS: *ypos -- increased by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void
move_down (int* ypos)
{
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the right pan border
     * while the bottom pixels of the maze are not on-screen.
     */
    if (++(*ypos) > game_info.map_y + SCROLL_Y_DIM -
	    BLOCK_Y_DIM * (PAN_BORDER + 1) && 
	game_info.map_y + SCROLL_Y_DIM < 
	    (2 * game_info.maze_y_dim + 1) * BLOCK_Y_DIM - SHOW_MIN) {
	/*
	 * Shift the logical view downwards by one pixel and draw the
	 * new line.
	 */
	set_view_window (game_info.map_x, ++game_info.map_y);
	(void)draw_horiz_line (SCROLL_Y_DIM - 1);
    }
}


/* 
 * move_left
 *   DESCRIPTION: Move the player right one pixel (assumed to be a legal move)
 *   INPUTS: xpos -- pointer to player's x position (pixel) in the maze
 *   OUTPUTS: *xpos -- decreased by one from initial value
 *   RETURN VALUE: none
 *   SIDE EFFECTS: pans display by one pixel when appropriate
 */
static void
move_left (int* xpos)
{
    /*
     * Move player by one pixel and check whether display should be panned.
     * Panning is necessary when the player moves past the left pan border
     * while the leftmost pixels of the maze are not on-screen.
     */
    if (--(*xpos) < game_info.map_x + BLOCK_X_DIM * PAN_BORDER && 
	game_info.map_x > SHOW_MIN) {
	/*
	 * Shift the logical view to the left by one pixel and draw the
	 * new line.
	 */
	set_view_window (--game_info.map_x, game_info.map_y);
	(void)draw_vert_line (0);
    }
}


/* 
 * unveil_around_player
 *   DESCRIPTION: Show the maze squares in an area around the player.
 *                Consume any fruit under the player.  Check whether
 *                player has won the maze level.
 *   INPUTS: (play_x,play_y) -- player coordinates in pixels
 *   OUTPUTS: none
 *   RETURN VALUE: 1 if player wins the level by entering the square
 *                 0 if not
 *   SIDE EFFECTS: draws maze squares for newly visible maze blocks,
 *                 consumed fruit, and maze exit; consumes fruit and
 *                 updates displayed fruit counts
 */
static int
unveil_around_player (int play_x, int play_y)
{
    int x = play_x / BLOCK_X_DIM; /* player's maze lattice position */
    int y = play_y / BLOCK_Y_DIM;
    int i, j;   /* loop indices for unveiling maze squares */

    /* Check for fruit at the player's position. */
    (void)check_for_fruit (x, y);

    /* Unveil spaces around the player. */
    for (i = -1; i < 2; i++)
	for (j = -1; j < 2; j++)
	    unveil_space (x + i, y + j);
    unveil_space (x, y - 2);
    unveil_space (x + 2, y);
    unveil_space (x, y + 2);
    unveil_space (x - 2, y);

    /* Check whether the player has won the maze level. */
    return check_for_win (x, y);
}


/* 
 * time_is_after 
 *   DESCRIPTION: Check whether one time is at or after a second time.
 *   INPUTS: t1 -- the first time
 *           t2 -- the second time
 *   OUTPUTS: none
 *   RETURN VALUE: 1 if t1 >= t2
 *                 0 if t1 < t2
 *   SIDE EFFECTS: none
 */
static int
time_is_after (struct timeval* t1, struct timeval* t2)
{
    if (t1->tv_sec == t2->tv_sec)
        return (t1->tv_usec >= t2->tv_usec);
    if (t1->tv_sec > t2->tv_sec)
        return 1;
    return 0;
}


static game_condition_t
game_loop ()
{
    /* 
     * Variables used to carry information between event loop ticks; see
     * initialization below for explanations of purpose.
     */
    int play_x, play_y, move_cnt, last_dir, dir, last_was_back;
    struct timeval start_time, tick_time, add_fruit_time;

    struct timeval cur_time; /* current time (during tick)      */
    cmd_t cmd;               /* command issued by input control */
    int open[NUM_DIRS];      /* directions open to motion       */

    /* 
     * The player starts at map position (1,1), which corresponds to
     * logical view position (BLOCK_X_DIM,BLOCK_Y_DIM).
     */
    play_x = BLOCK_X_DIM;
    play_y = BLOCK_Y_DIM;

    /* 
     * When moving, move_cnt tracks moves remaining between maze 
     * squares; when not moving, it should be 0.
     */
    move_cnt = 0;

    /* 
     * Initialize the last direction moved to up.  The last direction of
     * motion is necessary with the two-button parallel port controller,
     * which allows only turn left, turn right, and turn around to be
     * expressed.
     */
    last_dir = DIR_UP;

    /* Initialize the current direction of motion to stopped. */
    dir = DIR_STOP;

    /* 
     * If the last command issued was to turn around; wait for controller
     * quiescence (i.e., TURN_NONE) to avoid rapidly bouncing back and 
     * forth.
     */
    last_was_back = 0;

    /* Record the starting time--assume success. */
    (void)gettimeofday (&start_time, NULL);

    /* Calculate the time at which the first event loop tick should occur. */
    tick_time = start_time;
    if ((tick_time.tv_usec += game_info.tick_usec) > 1000000) {
	tick_time.tv_sec++;
	tick_time.tv_usec -= 1000000;
    }

    /* Calculate the time at which the first new fruit should be added. */
    add_fruit_time = start_time;
    add_fruit_time.tv_sec += game_info.time_to_first_fruit;

    /* 
     * Show the maze around the player's original position, ignoring an
     * immediate 'game won' situation, since the player can return to the
     * exit if an immediate win is somehow possible.
     */
    (void)unveil_around_player (play_x, play_y);

    /* The main event loop. */
    while (1) {
	/* 
	 * Update the screen.  We first draw all temporary entities,
	 * including the player and the floating text, then show the
	 * screen, and finally undraw the temporary entities in reverse
	 * order.
	 */
	draw_full_block (play_x, play_y, get_player_block (last_dir));
	show_screen ();
	/*
	 * You can just blat down the map squares around the player again
	 * to erase the player, but masking is more elegant (and required
	 * for full credit).  To name one drawback of not masking, you will 
	 * change the background around the player.
	 */

	/*
	 * Wait for tick.  The tick defines the basic timing of our
	 * event loop, and is the minimum amount of time between events.
	 */
	do {
	    if (gettimeofday (&cur_time, NULL) != 0) {
		/* Panic!  (should never happen) */
		clear_mode_X ();
		shutdown_input ();
		perror ("gettimeofday");
		exit (3);
	    }
	} while (!time_is_after (&cur_time, &tick_time));

	/*
	 * Advance the tick time.  If we missed one or more ticks completely, 
	 * i.e., if the current time is already after the time for the next 
	 * tick, just skip the extra ticks and advance the clock to the one
	 * that we haven't missed.
	 */
	do {
	    if ((tick_time.tv_usec += game_info.tick_usec) > 1000000) {
		tick_time.tv_sec++;
		tick_time.tv_usec -= 1000000;
	    }
	} while (time_is_after (&cur_time, &tick_time));

	/*
	 * Handle asynchronous events.  These events use real time rather
	 * than tick counts for timing, although the real time is rounded
	 * off to the nearest tick by definition.
	 */
	if (time_is_after (&cur_time, &add_fruit_time)) {
	    if (add_a_fruit (game_info.map_x, game_info.map_y) == 10)
		return GAME_LOST;
	    add_fruit_time.tv_sec += game_info.time_between_fruits;
	}

	/* Handle synchronous events--in this case, only player motion. */
	
	/*
	 * First, determine the current command.  Adjust it according 
	 * to rules of motion:
	 *   1) turning back twice requires releasing the controller
	 *      between events
	 *   2) left and right turns are only allowed when aligned with
	 *      the maze
	 */
	cmd = get_command (last_dir);
	if (cmd == CMD_QUIT)
	    return GAME_QUIT;
	if (cmd == TURN_BACK) {
	    if (last_was_back)
	        cmd = TURN_NONE;
	    else
	        last_was_back = 1;
	} else if (cmd == TURN_NONE)
	    /* Need to release controller between reversals. */
	    last_was_back = 0;
	else if (move_cnt != 0)
	    /* Left and right turns only allowed when aligned with maze. */
	    cmd = TURN_NONE;

	/* 
	 * Now decide the direction of motion.
	 *    1) motion continues in the same direction unless a command
	 *       was issued
	 *    2) turn back commands always work (after last stage's filter)
	 *    3) left and right commands are ignored unless the path is
	 *       open
	 */
	if (cmd == TURN_BACK) {
	    dir = (last_dir + TURN_BACK) % 4;
	    /* Calculate the number of pixels back to the last square. */ 
	    if (move_cnt > 0) {
		if (dir == DIR_UP || dir == DIR_DOWN)
		    move_cnt = BLOCK_Y_DIM - move_cnt;
		else
		    move_cnt = BLOCK_X_DIM - move_cnt;
	    }
	}
	if (move_cnt == 0) {
	    /* 
	     * The player has reached a new maze square; unveil nearby maze
	     * squares and check whether the player has won the level.
	     */
	    if (unveil_around_player (play_x, play_y))
	        return GAME_WON;

	    /* Record directions open to motion. */
	    find_open_directions (play_x / BLOCK_X_DIM, play_y / BLOCK_Y_DIM,
			          open);
	    
#if 1 /* keyboard-controlled player */
	    if (cmd == TURN_LEFT || cmd == TURN_RIGHT) {
		int new_dir = (last_dir + cmd) % 4;
		if (open[new_dir])
		    dir = new_dir;
	    }
	    /* 
	     * The direction may not be open to motion...
	     *    1) ran into a wall
	     *    2) initial direction and its opposite both face walls
	     */
	    if (dir != DIR_STOP) {
	        if (!open[dir])
		    dir = DIR_STOP;
	        else if (dir == DIR_UP || dir == DIR_DOWN)
		    move_cnt = BLOCK_Y_DIM;
	        else
		    move_cnt = BLOCK_X_DIM;
	    }
#else /* computer-controlled player */
	    {
		int i, choices, pick;

		/* Don't turn around unless it's necessary. */
		if (dir != DIR_STOP)
		    open[(dir + 2) % 4] = 0;
		/* Count open directions. */
		for (i = 0, choices = 0; i < 4; i++)
		    choices += open[i];
		/* If necessary, turn around. */
		if (choices == 0)
		    dir = (dir + 2) % 4;
		else {
		    /* 
		     * Pick an open direction at random.  The necessary
		     * call to srandom () was made in maze construction. 
		     */
		    pick = (random () % choices);
		    for (dir = 0; !open[dir] || pick; dir++)
			if (open[dir])
			    pick--;
		}
		/* How many pixels must be moved to reach the next square? */
	        if (dir == DIR_UP || dir == DIR_DOWN)
		    move_cnt = BLOCK_Y_DIM;
	        else
		    move_cnt = BLOCK_X_DIM;
	    }
#endif
	}

	if (dir != DIR_STOP) {
	    /* move in chosen direction */
	    last_dir = dir;
	    move_cnt--;
	    switch (dir) {
		case DIR_UP:    move_up (&play_y);    break;
		case DIR_RIGHT: move_right (&play_x); break;
		case DIR_DOWN:  move_down (&play_y);  break;
		case DIR_LEFT:  move_left (&play_x);  break;
	    }
	}

    } /* end of the main event loop */
}


/* 
 * main
 *   DESCRIPTION: Play the maze game.
 *   INPUTS: none (command line arguments are ignored)
 *   OUTPUTS: none
 *   RETURN VALUE: 0 on success, 3 in panic situations
 */
int
main ()
{
    int level;              /* current maze level */
    game_condition_t game;  /* outcome of playing */

    /* Provide some protection against fatal errors. */
    clean_on_signals ();

    /* Perform sanity checks. */
    if (0 != sanity_check ()) {
	PANIC ("failed sanity checks");
    }

    /* Start mode X. */
    if (0 != set_mode_X (fill_horiz_buffer, fill_vert_buffer)) {
	PANIC ("cannot initialize mode X");
    }
    push_cleanup ((cleanup_fn_t)clear_mode_X, NULL); {

	/* Initialize the keyboard and/or Tux controller. */
	if (0 != init_input ()) {
	    PANIC ("cannot initialize input");
	}
	push_cleanup ((cleanup_fn_t)shutdown_input, NULL); {

	    /* Loop over levels until a level is lost or quit. */
	    for (level = 1, game = GAME_WON; 
		 level <= MAX_LEVEL && game == GAME_WON; level++) {

		/* 
		 * Prepare for the level.  If we fail, just let 
		 * the player win. 
		 */
		if (prepare_maze_level (level) != 0)
		    break;

		/* Play the new level. */
		game = game_loop ();
	    }

	} pop_cleanup (1);

    } pop_cleanup (1);

    /* Print a message about the outcome. */
    switch (game) {
	case GAME_WON: printf ("You win the game!  CONGRATULATIONS!\n"); break;
	case GAME_LOST: printf ("Sorry, you lose...\n"); break;
	case GAME_QUIT: printf ("Quitter!\n"); break;
    }

    /* Return success. */
    return 0;
}


#if !defined(NDEBUG)
/* 
 * sanity_check 
 *   DESCRIPTION: Perform checks on changes to constants and enumerated values.
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: 0 if checks pass, -1 if any fail
 *   SIDE EFFECTS: none
 */
static int 
sanity_check ()
{
    /* 
     * Automatically detect when fruits have been added in blocks.h
     * without allocating enough bits to identify all types of fruit
     * uniquely (along with 0, which means no fruit).
     */
    if (((2 * LAST_MAZE_FRUIT_BIT) / MAZE_FRUIT_1) < NUM_FRUIT_TYPES + 1) {
	puts ("You need to allocate more bits in maze_bit_t to encode fruit.");
	return -1;
    }
    return 0;
}
#endif /* !defined(NDEBUG) */
