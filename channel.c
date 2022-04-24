#include "channel.h"
#include "buffer.h"
#include "linked_list.h"
// #include "thread.h"

// Creates a new channel with the provided size and returns it to the caller
// A 0 size indicates an unbuffered channel, whereas a positive size indicates a buffered channel
chan_t* channel_create(size_t size)
{
  // printf("channel size is %ld\n--------------------\n", size);
    chan_t* chan = malloc(sizeof(chan_t));
    chan -> buffer = buffer_create(size);
    chan -> open = true;
    pthread_mutex_init(&chan -> mutex, NULL);
    pthread_cond_init(&chan -> recv_cv, NULL);
    pthread_cond_init(&chan -> send_cv, NULL);
    chan -> recv_sel_nodes = list_create();
    chan -> send_sel_nodes = list_create();
    pthread_mutex_init(&chan -> recv_sel_mutex, NULL);
    pthread_mutex_init(&chan -> send_sel_mutex, NULL);
    return chan;
}

// Writes data to the given channel
// This can be both a blocking call i.e., the function only returns on a successful completion of send (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is full (blocking = false)
// In case of the blocking call when the channel is full, the function waits till the channel has space to write the new data
// Returns SUCCESS for successfully writing data to the channel,
// WOULDBLOCK if the channel is full and the data was not added to the buffer (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_send(chan_t* channel, void* data, bool blocking)
{
  //printf("send\n");
    /*
    if (!channel -> open){
        pthread_cond_broadcast(&channel -> send_cv);
        pthread_mutex_unlock(&channel -> mutex);
        return CLOSED_ERROR;
    }
    */
    bool add_succ;
    
    if (blocking) {
        //printf("\nsend blocking\n");
        
        pthread_mutex_lock(&channel -> mutex);
        
        if (!channel -> open) {
            pthread_cond_broadcast(&channel -> send_cv);
            pthread_mutex_unlock(&channel -> mutex);
            return CLOSED_ERROR;
        }
        
        add_succ = buffer_add(data, channel -> buffer);
        while (!add_succ) {
	  //printf("  waiting to send\n");
            pthread_cond_wait(&channel -> send_cv, &channel -> mutex);
            //printf("  resuming waiting thread\n");
            
            if (!channel -> open) {
              //printf("  channel is closed\n");
                pthread_cond_broadcast(&channel -> send_cv);
                pthread_mutex_unlock(&channel -> mutex);
                return CLOSED_ERROR;
            }
            
            add_succ = buffer_add(data, channel -> buffer);
	    /*
	    if (!add_succ)
	      printf("    sending not successful\n");
            else printf("    sending successful\n");
	    */
        }
        
        //printf("sent message\n");
        pthread_cond_signal(&channel -> recv_cv);
        
        
        pthread_mutex_lock(&channel -> recv_sel_mutex);
        if (list_count(channel -> recv_sel_nodes) != 0) {
	  // printf("Waking all receivers (%li) (blocking)\n", list_count(channel -> recv_sel_nodes));
            list_foreach(channel -> recv_sel_nodes, (void*)sem_post);
        }
        pthread_mutex_unlock(&channel -> recv_sel_mutex);
        
        pthread_mutex_unlock(&channel -> mutex);
        
        //printf("send success of %s\n", (char*)data);
        return SUCCESS;
    } 
    
    else {
      //printf("\nsend non-blocking\n");
        pthread_mutex_lock(&channel -> mutex);

        if (!channel -> open){
            pthread_cond_broadcast(&channel -> send_cv);
            pthread_mutex_unlock(&channel -> mutex);
            return CLOSED_ERROR;
    	}

        add_succ = buffer_add(data, channel -> buffer);
            
        pthread_cond_signal(&channel -> recv_cv);
        
        
        pthread_mutex_lock(&channel -> recv_sel_mutex);
        if (list_count(channel -> recv_sel_nodes) != 0) {
	  //    printf("Waking all receivers\n");
            list_foreach(channel -> recv_sel_nodes, (void*)sem_post);
        }
        pthread_mutex_unlock(&channel -> recv_sel_mutex);
        
        pthread_mutex_unlock(&channel -> mutex);
        
        if (!add_succ)
            return WOULDBLOCK;
            
        return SUCCESS;
    }
    
}

// Reads data from the given channel and stores it in the function’s input parameter, data (Note that it is a double pointer).
// This can be both a blocking call i.e., the function only returns on a successful completion of receive (blocking = true), and
// a non-blocking call i.e., the function simply returns if the channel is empty (blocking = false)
// In case of the blocking call when the channel is empty, the function waits till the channel has some data to read
// Returns SUCCESS for successful retrieval of data,
// WOULDBLOCK if the channel is empty and nothing was stored in data (non-blocking calls only),
// CLOSED_ERROR if the channel is closed, and
// OTHER_ERROR on encountering any other generic error of any sort
enum chan_status channel_receive(chan_t* channel, void** data, bool blocking)
{
    /*
    if (!channel -> open){
        pthread_mutex_unlock(&channel -> mutex);
        return CLOSED_ERROR;
    }
    */

    void* read_succ;

    if (blocking) {
        //printf("\nblocking recv\n");
        
        pthread_mutex_lock(&channel -> mutex);
        
        if (!channel -> open) {
            pthread_cond_broadcast(&channel -> recv_cv);
            pthread_mutex_unlock(&channel -> mutex);
            return CLOSED_ERROR;
        }
        
        read_succ = buffer_remove(channel-> buffer);
        //printf("here\n");
        while (read_succ == BUFFER_EMPTY) {
            //printf("  recv wait\n");
            pthread_cond_wait(&channel -> recv_cv, &channel -> mutex);
            //printf("  recv wakeup\n");
            if (!channel -> open){
                pthread_cond_broadcast(&channel -> recv_cv);
                pthread_mutex_unlock(&channel -> mutex);
                return CLOSED_ERROR;
            }
            //printf("    channel still open\n");
            read_succ = buffer_remove(channel-> buffer);
        }
        //if (read_succ)
	  //printf("  read success\n");
        
        *data = read_succ;
        
        pthread_cond_signal(&channel -> send_cv);
        
        pthread_mutex_lock(&channel -> send_sel_mutex);
        if (list_count(channel -> send_sel_nodes) != 0) {
	    //printf("Waking all senders (%li) (blocking)\n", list_count(channel -> send_sel_nodes));
            list_foreach(channel -> send_sel_nodes, (void*)sem_post);
        }
        pthread_mutex_unlock(&channel -> send_sel_mutex);
        
        pthread_mutex_unlock(&channel -> mutex);
        
        return SUCCESS;
    } 
    
    else {
        //printf("non-blocking recv\n");
        
        pthread_mutex_lock(&channel -> mutex);

        if (!channel -> open){
            pthread_cond_broadcast(&channel -> recv_cv);
            pthread_mutex_unlock(&channel -> mutex);
            return CLOSED_ERROR;
        }

        read_succ = buffer_remove(channel-> buffer);
        
        pthread_cond_signal(&channel -> send_cv);
        //printf("  sent signal\n");
        
        pthread_mutex_lock(&channel -> send_sel_mutex);
        if (list_count(channel -> send_sel_nodes) != 0) {
            //printf("Waking all senders\n");
            list_foreach(channel -> send_sel_nodes, (void*)sem_post);
        }
        pthread_mutex_unlock(&channel -> send_sel_mutex);
        
        if (read_succ == BUFFER_EMPTY) {
            pthread_mutex_unlock(&channel -> mutex);
            //printf("    would block\n");
            return WOULDBLOCK;
        }
        //printf("  read success\n");
        
        *data = read_succ;
        
        pthread_mutex_unlock(&channel -> mutex);
        //printf("unlocked\n");
            
        return SUCCESS;
    }
}

// Closes the channel and informs all the blocking send/receive/select calls to return with CLOSED_ERROR
// Once the channel is closed, send/receive/select operations will cease to function and just return CLOSED_ERROR
// Returns SUCCESS if close is successful,
// CLOSED_ERROR if the channel is already closed, and
// OTHER_ERROR in any other error case
enum chan_status channel_close(chan_t* channel)
{
    
    pthread_mutex_lock(&channel -> mutex);
    
    // printf("\nchannel close\n");
    if (!channel -> open){
        pthread_mutex_unlock(&channel -> mutex);
        return CLOSED_ERROR;
    }
    
    
    channel -> open = false;
    //printf("closed channel flag\n");
    pthread_cond_broadcast(&channel -> send_cv);
    pthread_cond_broadcast(&channel -> recv_cv);
    
    list_foreach(channel -> send_sel_nodes, (void*)sem_post);
    list_foreach(channel -> recv_sel_nodes, (void*)sem_post);
    
    pthread_mutex_unlock(&channel -> mutex);
    
    return SUCCESS;
}

// Frees all the memory allocated to the channel
// The caller is responsible for calling channel_close and waiting for all threads to finish their tasks before calling channel_destroy
// Returns SUCCESS if destroy is successful,
// DESTROY_ERROR if channel_destroy is called on an open channel, and
// OTHER_ERROR in any other error case
enum chan_status channel_destroy(chan_t* channel)
{
    /* IMPLEMENT THIS */
    //printf("\nchannel destroy\n");
    if (channel -> open)
        return DESTROY_ERROR;
    buffer_free(channel->buffer);
    list_destroy(channel -> recv_sel_nodes);
    list_destroy(channel -> send_sel_nodes);
    pthread_mutex_destroy(&channel -> mutex);
    pthread_cond_destroy(&channel -> recv_cv);
    pthread_cond_destroy(&channel -> send_cv);
    pthread_mutex_destroy(&channel -> recv_sel_mutex);
    pthread_mutex_destroy(&channel -> send_sel_mutex);
    free(channel);
    return SUCCESS;
}


void subscribe_channels(select_t* channel_list, size_t channel_count, sem_t* sem) {
    for (int i = 0; i < channel_count; i++) {
        select_t* sel = &channel_list[i];
        chan_t* chan = sel -> channel;
        if (sel -> is_send) {
            pthread_mutex_lock(&chan -> send_sel_mutex);
	    //      printf("insert %p to send_sel\n", sem);
            list_insert(chan -> send_sel_nodes, sem);
            pthread_mutex_unlock(&chan -> send_sel_mutex);
        }
        else {
            pthread_mutex_lock(&chan -> recv_sel_mutex);
            //printf("insert %p to recv_sel\n", sem);
            list_insert(chan -> recv_sel_nodes, sem);
	    // printf("now has %li nodes\n", list_count(chan -> recv_sel_nodes));
            pthread_mutex_unlock(&chan -> recv_sel_mutex);
        }
    }
}

void unsubscribe_channels(select_t* channel_list, size_t channel_count, sem_t* sem) {
  //printf("must unsubscribe %lu channels\n", channel_count);
    for (int i = 0; i < channel_count; i++) {
        printf("%i\n", i);
        select_t* sel = &channel_list[i];
        chan_t* chan = sel -> channel;
        if (sel -> is_send) {
            pthread_mutex_lock(&chan -> send_sel_mutex);
	    //        printf("delete sem %p\n", sem);
            list_node_t* node = list_find(chan -> send_sel_nodes, sem);
            list_remove(chan -> send_sel_nodes, node);
            //printf("now has %li nodes\n", list_count(chan -> send_sel_nodes));
            pthread_mutex_unlock(&chan -> send_sel_mutex);
        }
        else {
            pthread_mutex_lock(&chan -> recv_sel_mutex);
            //printf("delete sem %p\n", sem);
            list_node_t* node = list_find(chan -> recv_sel_nodes, sem);
            list_remove(chan -> recv_sel_nodes, node);
            //printf("now has %li nodes\n", list_count(chan -> recv_sel_nodes));
            pthread_mutex_unlock(&chan -> recv_sel_mutex);
        }
    }
}


// Takes an array of channels, channel_list, of type select_t and the array length, channel_count, as inputs
// This API iterates over the provided list and finds the set of possible channels which can be used to invoke the required operation (send or receive) specified in select_t
// If multiple options are available, it selects the first option and performs its corresponding action
// If no channel is available, the call is blocked and waits till it finds a channel which supports its required operation
// Once an operation has been successfully performed, select should set selected_index to the index of the channel that performed the operation and then return SUCCESS
// In the event that a channel is closed or encounters any error, the error should be propagated and returned through select
// Additionally, selected_index is set to the index of the channel that generated the error
enum chan_status channel_select(size_t channel_count, select_t* channel_list, size_t* selected_index)
{
  //printf("\ncalling select()\n");
    sem_t s;
    sem_init(&s, 0, 0);
    
    subscribe_channels(channel_list, channel_count, &s);
    //printf("  subscribed channels\n");
    
    enum chan_status succ;
    while (true) {
        for (int i = 0; i < channel_count; i++) {
            select_t *sel = &channel_list[i];
            chan_t *chan = sel -> channel;
            
            if (sel -> is_send){
                succ = channel_send(chan, sel -> data, false);
                if (succ == SUCCESS) {
                    *selected_index = (size_t)i;
                    unsubscribe_channels(channel_list, channel_count, &s);
                    return SUCCESS;
                }
                else if (succ == WOULDBLOCK) {
                    continue;
                }
                else {
                    *selected_index = (size_t)i;
                    unsubscribe_channels(channel_list, channel_count, &s);
                    return succ;
                }
                
            }
            else {
                succ = channel_receive(chan, &sel -> data, false);
                if (succ == SUCCESS) {
                    *selected_index = (size_t)i;
		    //             printf("selct_t data is %s\n", (char*)sel -> data);
                    unsubscribe_channels(channel_list, channel_count, &s);
                    //printf("unsubscribed channels\n");
                    return SUCCESS;
                }
                else if (succ == WOULDBLOCK) {
                    continue;
                }
                else {
                    *selected_index = (size_t)i;
                    unsubscribe_channels(channel_list, channel_count, &s);
                    return succ;
                }
            }
        }
        
        //printf("  no ops found, must wait\n");
        sem_wait(&s);
        //printf("  select() successfully awoken\n");
        
    }
    return SUCCESS;
}
