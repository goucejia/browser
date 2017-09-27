/* CSci4061 F2016 Assignment 2
* date: 10/17/2016
* name: Garrett Sundin, Brent Higgins, Yu Fang
* ids: 4299872, 4725412, 5094239
* x500s: sundi024, higgi295, fangx174 */

#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_TAB 100
extern int errno;

/*
 * Name:                uri_entered_cb
 * Input arguments:     'entry'-address bar where the url was entered
 *                      'data'-auxiliary data sent along with the event
 * Output arguments:    none
 * Function:            When the user hits the enter after entering the url
 *                      in the address bar, 'activate' event is generated
 *                      for the Widget Entry, for which 'uri_entered_cb'
 *                      callback is called. Controller-tab captures this event
 *                      and sends the browsing request to the ROUTER (parent)
 *                      process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data) {
 	if(data == NULL) {
 		return;
 	}
 	browser_window * b_window = (browser_window *)data;
 	// This channel has pipes to communicate with ROUTER.
 	comm_channel channel = b_window->channel;
 	// Get the tab index where the URL is to be rendered
 	int tab_index = query_tab_id_for_request(entry, data);
 	if(tab_index <= 0 || tab_index >= MAX_TAB) {
 		fprintf(stderr, "Invalid tab index (%d).\n", tab_index);
 		return;
 	}
 	// Get the URL.
 	char * uri = get_entered_uri(entry);
 	
 	// Append your code here
 	// ------------------------------------
 	// * Prepare a NEW_URI_ENTERED packet to send to ROUTER (parent) process.
 	
 	child_req_to_parent buffer;
 	buffer.type = NEW_URI_ENTERED;
 	memcpy(buffer.req.uri_req.uri, uri, strlen(uri) + 1);  // Copy entered URI into the packet
 	buffer.req.uri_req.render_in_tab = tab_index;  // Copy which tab to render to into the packet

 	// * Set up pipes to send the packet to the router
 	// Configure pipes to write from child and read from parent
 	//close(channel.child_to_parent_fd[0]);  //Close 'read' end of pipe C2P pipe
 	//close(channel.parent_to_child_fd[1]);  //Close 'write' end of P2C pipe

 	//if (pipe(channel.child_to_parent_fd) < 0)  //Create a pipe with the pipe call.
 	//{
 		//perror("Failed to create pipe");
 	//}
 	//else
 	//{
 		//// * Send the url and tab index to ROUTER
		//write(channel.child_to_parent_fd[1], &buffer, sizeof(buffer));  // Write the data from the buffer to the router through the pipe
 	//}
	int n = write(channel.child_to_parent_fd[1], &buffer, sizeof(buffer));
	//fprintf(stderr, "%d \n", n);
 	// Seal off pipes completely after the message is sent
 	//close(channel.child_to_parent_fd[1]);  //Close 'write' end of pipe C2P pipe
 	//close(channel.parent_to_child_fd[0]);  //Close 'read' end of P2C pipe
 	// ------------------------------------
}
void create_new_tab_cb(GtkButton *button, gpointer data) {
	if(data == NULL) {
		return;
	}
	// This channel has pipes to communicate with ROUTER.
	comm_channel channel = ((browser_window*)data)->channel;
	
	// Append your code here.
	// ------------------------------------
	// * Send a CREATE_TAB message to ROUTER (parent) process.
	// ------------------------------------
    
    child_req_to_parent buffer;
	buffer.type = CREATE_TAB;
	int n=write(channel.child_to_parent_fd[1], &buffer, sizeof(buffer));
}

/*
 * Name:                url_rendering_process
 * Input arguments:     'tab_index': URL-RENDERING tab index
 *                      'channel': Includes pipes to communctaion with
 *                      Router process
 * Output arguments:    none
 * Function:            This function will make a URL-RENDRERING tab Note.
 *                      You need to use below functions to handle tab event.
 *                      1. process_all_gtk_events();
 *                      2. process_single_gtk_event();
*/
int url_rendering_process(int tab_index, comm_channel *channel) {
	// Don't forget to close pipe fds which are unused by this process
	
 	close(channel->child_to_parent_fd[0]);  //Close 'read' end of P2C pipe
	close(channel->parent_to_child_fd[1]);  //Close 'write' end of P2C pipe
	browser_window * b_window = NULL;
	// Create url-rendering window
	create_browser(URL_RENDERING_TAB, tab_index, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	child_req_to_parent req;
	while (1) {
		child_req_to_parent buffer;
		// Handle one gtk event, you don't need to change it nor understand what it does.
		process_single_gtk_event();
		// Poll message from ROUTER
		// It is unnecessary to poll requests unstoppably, that will consume too much CPU time
		// Sleep some time, e.g. 1 ms, and render CPU to other processes
		usleep(1000);
		// Append your code here
		// Try to read data sent from ROUTER

		int flag;
		flag = fcntl((*channel).parent_to_child_fd[0], F_GETFL, 0);
		flag = flag | O_NONBLOCK;
		fcntl((*channel).parent_to_child_fd[0], F_SETFL, flag);
		int n;
		n = read(channel->parent_to_child_fd[0], &buffer, sizeof(buffer));
		if (n > 0)
		{
			//fprintf(stderr, "%d ", n);
			 //Otherwise, check message type:
				   //* NEW_URI_ENTERED
				     //** call render_web_page_in_tab(req.req.uri_req.uri, b_window);
			if (buffer.type == NEW_URI_ENTERED)
			{
				render_web_page_in_tab(buffer.req.uri_req.uri, b_window);
			}
			//   * TAB_KILLED
			//     ** call process_all_gtk_events() to process all gtk events and jump out of the loop
			else if (buffer.type == TAB_KILLED)
			{
				process_all_gtk_events();
				// Close off pipes completetly if process ends
				close(channel->parent_to_child_fd[0]);  //Close 'read' end of pipe C2P pipe
				close(channel->child_to_parent_fd[1]);  //Close 'write' end of pipe C2P pipe
				break;  // Break out of while loop
			}
			//   * CREATE_TAB or unknown message type
			//     ** print an error message and ignore it
			else
			{
				fprintf(stderr, "%d", tab_index);
				perror("CREATE_TAB or unknown message type.");
			}
		}
	  // Handle read error, e.g. what if the ROUTER has quit unexpected?
		else if(errno != EAGAIN)
		{
			//fprintf(stderr, "hi");
			// Close off pipes completetly if process ends
			close(channel->parent_to_child_fd[0]);  //Close 'read' end of pipe C2P pipe
			close(channel->child_to_parent_fd[1]);  //Close 'write' end of pipe C2P pipe
			perror("Nothing read from ROUTER.");
		}

	}

	return 0;
}
/*
 * Name:                controller_process
 * Input arguments:     'channel': Includes pipes to communctaion with
 *                      Router process
 * Output arguments:    none
 * Function:            This function will make a CONTROLLER window and
 *                      be blocked until the program terminates.
 */
int controller_process(comm_channel *channel) {
	// Do not need to change code in this function
	close(channel->child_to_parent_fd[0]);
	close(channel->parent_to_child_fd[1]);
	browser_window * b_window = NULL;
	// Create controler window
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(create_new_tab_cb), G_CALLBACK(uri_entered_cb), &b_window, channel);
	show_browser();
	return 0;
}

/*
 * Name:                router_process
 * Input arguments:     none
 * Output arguments:    none
 * Function:            This function implements the logic of ROUTER process.
 *                      It will first create the CONTROLLER process  and then
 *                      polling request messages from all ite child processes.
 *                      It does not need to call any gtk library function.
 */
int router_process() {
	comm_channel *channel[MAX_TAB] = {NULL};
	int tab_pid_array[MAX_TAB] = {0}; // You can use this array to save the pid
	                                  // of every child process that has been
					  // created. When a chile process receives
					  // a TAB_KILLED message, you can call waitpid()
					  // to check if the process is indeed completed.
					  // This prevents the child process to become
					  // Zombie process. Another usage of this array
					  // is for bookkeeping which tab index has been
					  // taken.
	// Append your code here
	// Prepare communication pipes with the CONTROLLER process
	comm_channel controller;
	channel[0] = &controller;
	if (pipe(controller.parent_to_child_fd)<0)
	{
		perror("Failed to create pipe");
	}
	if (pipe((controller).child_to_parent_fd)<0)
	{
		perror("Failed to create pipe");
	}
	
	// Fork the CONTROLLER process
	//   call controller_process() in the forked CONTROLLER process
	// Don't forget to close pipe fds which are unused by this process
	// Poll child processes' communication channels using non-blocking pipes.
	// Before any other URL-RENDERING process is created, CONTROLLER process
	// is the only child process. When one or more URL-RENDERING processes
	// are created, you would also need to poll their communication pipe.
	//   * sleep some time if no message received
	//   * if message received, handle it:
	tab_pid_array[0] = fork();
	if (tab_pid_array[0] == 0)
	{
		controller_process(channel[0]);
		exit(0);
	}
	else if(tab_pid_array[0] > 0)
	{
		
		close(channel[0]->child_to_parent_fd[1]);
		close(channel[0]->parent_to_child_fd[0]);
		while(1)
		{
			
			int i = 0;
			for(i; i < MAX_TAB; i++)
			{
				
				
				if(channel[i] != NULL)
				{
					//fprintf(stderr,"%d || ",i);
					//fprintf(stderr,"hi \n");
					int flag;
					child_req_to_parent buffer;
					flag = fcntl((*channel[i]).child_to_parent_fd[0], F_GETFL, 0);
					flag = flag | O_NONBLOCK;
					fcntl((*channel[i]).child_to_parent_fd[0], F_SETFL, flag);
					int nread = 0;
					nread = read((*channel[i]).child_to_parent_fd[0], &buffer, sizeof(buffer));
					//fprintf(stderr, "%d ", nread);
					if(nread != -1)
					{
						
						
						//     ** CREATE_TAB:
						//
						//        Prepare communication pipes with the new URL-RENDERING process
						//        Fork the new URL-RENDERING process
						//
						if(buffer.type == CREATE_TAB)
						{
							int tab_num;
							for(int j = 1; j < MAX_TAB; j++)
							{
								if(channel[j] == NULL)
								{
									tab_num = j;
									break;
								}
							}
							
							comm_channel tab;				
							channel[tab_num] = (comm_channel *)malloc(sizeof(comm_channel));			
						
							if (pipe(channel[tab_num]->parent_to_child_fd)<0)
							{
								perror("Failed to create pipe");
							}
							if (pipe(channel[tab_num]->child_to_parent_fd)<0)
							{
								perror("Failed to create pipe");
							}
							
							tab_pid_array[tab_num] = fork();
							if (tab_pid_array[tab_num] == 0)
							{
								url_rendering_process(tab_num, channel[tab_num]);
								exit(0);
							}
							else if(i < 0)
							{
								perror("failed to fork:");
								exit(1);
							}
							else
							{
								close(tab.child_to_parent_fd[1]);
								close(tab.parent_to_child_fd[0]);
							}
						}
						
						//     ** NEW_URI_ENTERED:
						//
						//        Send TAB_KILLED message to the URL-RENDERING process in which
						//        the new url is going to be rendered
						//
						else if (buffer.type == NEW_URI_ENTERED)
						{
							int tab_num = buffer.req.uri_req.render_in_tab;
							
							if(channel[tab_num] != NULL)
							{
								fprintf(stderr, "%d ", tab_num);
								write(channel[tab_num]->parent_to_child_fd[1], &buffer, sizeof(buffer));
							}
							else
							{
								fprintf(stderr,"Tab not open: (%d)\n", tab_num);
							}
						}
						
						//     ** TAB_KILLED:
						//
						else if (buffer.type == TAB_KILLED)
						{
							int status;
							int tab_num =buffer.req.killed_req.tab_index;
							fprintf(stderr, "killed %d\n", tab_num);
							
							//        If the killed process is the CONTROLLER process
							//        *** send TAB_KILLED messages to kill all the URL-RENDERING processes
							//        *** call waitpid on every child URL-RENDERING processes
							//        *** self exit
							if (tab_num != 0)
							{
								write((*channel[tab_num]).parent_to_child_fd[1], &buffer, sizeof(buffer));
								waitpid(tab_pid_array[tab_num], &status, 0);
								channel[tab_num] = NULL;
							}
							
							//        If the killed process is a URL-RENDERING process
							//        *** send TAB_KILLED to the URL-RENDERING
							//        *** call waitpid on every child URL-RENDERING processes
							//        *** close pipes for that specific process
							//        *** remove its pid from tab_pid_array[]
							else
							{
								for(int j = 1; j < MAX_TAB; j++)
								{
									//fprintf(stderr, "%d ", j);
									if(channel[j] != NULL)
									{
										fprintf(stderr, "%d \n", j);
										write((*channel[j]).parent_to_child_fd[1], &buffer, sizeof(buffer));
										waitpid(tab_pid_array[j], NULL, 0);
										//fprintf(stderr,"hi\n");
										free(channel[j]);
										channel[j] = NULL;
									}
								}
								close(channel[0]->child_to_parent_fd[0]);
								close(channel[0]->parent_to_child_fd[1]);
								return(0);
							}
						}
					}
					else
					{
						//fprintf(stderr, "%d", nread);
					}
				}
			}

			usleep(1000);
		}
	}
	else
	{
		perror("failed to fork:");
		exit(1);
	}

	return 0;
}

int main() {
	return router_process();
}
