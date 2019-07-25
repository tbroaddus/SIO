# ScrybeIO

ScrybeIO is a combination of classes and functions intended to be used on the
Scrybe Block Chain.

## IODevice

IODevice is a class under the ScrybeIO namespace that is intended to handle
serverside socket management with multiple threads by implementing epoll event
loops and non-blocking sockets. There are two private friend structs embedded within
the class: acceptor and handler. Both have their own respective epoll
structures. Acceptor observers the listening socket for new connections and
adds new client connections to the epoll structure observed by the Handler(s).
Handler recieves requests from the clients and passes the requests to the
handle function provided by the user in the IODevice's constructor.

There are 7 functions that the user has access to with an IODevice object:
set\_listen(), start(), stop(), pause(), resume(), reset(),
check\_master\_fail(). 

*	set\_listen() creates a new socket, binds the socket to a designated port, and
	sets the socket to listen for new connections. 0 is returned on success, -1 is
	returned on failure along with a cerr message.

*	start() creates threads with the processes provided by the acceptor and handler
	structs. The threads are stored in the thread\_vec vector. 0 is returned on 
	success, -1 is returned on failure along with a cerr message.

*	stop() should be considered as a hard stop that ends the services provided by
	the IODevice. Only reset() can be called after stop(). 0 is returned on
	success, -1 is returned in the case that a master fail was detected when stop()
	was called meaning that the threads terminated prematurely.

*	pause() "pauses" the services provided by the IODevice object. pause() can 
	only be called when IODevice is running (after start() was called). This
	terminates the acceptor and handler threads, but the client sockets are
	retained and the listening socket continues to listen for new connections to
	add. resume() or reset() can be called after pause(). 0 is returned on
	success, -1 is returned when the IODevice object is not running. 

*	reset() resets the entire IODevice object. All previous connections are lost
	along with the listening socket's connection queue. 0 is returned on success, -1
	is returned when the IODevice object is running (must call stop() or pause()
	before reset()). 

*	check\_master\_fail() allows the main thread to do other work while the
	IODevice object handles client requests. At anypoint in execution on the main
	thread, the status of the IODevice can be checked allowing the server to handle
	the situation in a manner defined by the user. 0 returns if there is a master
	fail present in the IODevice object (IODevice stopped prematurely), -1 is returned
	if there is not a master fail present (IODevice is running properly). It is 
	advised to call stop() in the event that a master fail is present within the IODevice 
	object. The user can then call reset(), set\_listen(), and start() to continue 
	handling client requests. Unfortunetly, all previous connections would be lost.

There are 10 macros that can be set:

*	THREADCOUNT defines the number of child threads to be created for the
	IODevice object. 2 is the minimum. There will only be one acceptor thread.
	Any additonal threads will be handler threads. Example: THREADCOUNT = 4
	will spawn 1 acceptor thread and 3 handler threads.

*	BUFFERSIZE defines the size of the buffer that requests are read into.






## IODevice Usage
