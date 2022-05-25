# Made by: Ofir Rubin

## OS Ex5 - Shared stack server w& processes & mmap

### Project structure
- Makefile: make all, client, test, clean | local shell was removed since it had no use.
- Client: Not updated since Ex4, Same structure [Run by `<client> <ipv4>` | Usage (in-app): `push <inp>` | `pop` | `top` ]
- Server: [Run by `<server>` | Usage: N/A] 
	Structure:
		Since it's made by processes which has seperate memory by default -
		I'm using mmap, the problem? mmap doesn't update additional shared address for forked processes.
		
		Since I didn't want to pre-allocate memory (waste of resources), I came up with this solution:
		Stack is saved in the main process and forked clients ignoring it.
		Since stack is LIFO, it means that anything but top element is inaccessable by definition.
		Thus main process may contain only few static addresses (shared values):
		'Shared stack statistics':
		- TOP Value
		- Is empty (since I can't free the shared memory)
		But input & output is required, so we need somewhere to save them:
		[input]
		- Shared input
		- Shared input size (so at least for each element in stack we don't need to use maximum input size)
		[output]
		- Shared output
		- Action result (integer status code)
		In addition, since we have limited resources -
		we need to manage them and avoid intefference between clients; Thus we use shared mutex.
		
		Finally we need somewhere to syncronize the functions:
		TOP function does not require anything but mutex since it is single direction action.
		PUSH & POP on the other hand is 2 way action: We send data for processing & need to wait for the result.
		
		For that reason I used signals; On the 'request' part I used SIGUSR1 as PUSH and SIGUSR2 as POP.
		By locking the memory for single client use, setting the shared input variables we can signal the main process to input
		the data into the stack & update the output for the shared memory (so every client process will be updated).
		Then we need to update the 'respond' of the action, for that part I assigned SIGUSR1 & SIGUSR2 as Complete/Failed and
		any other signal as -1 (Unknown or perhaps not requested somehow).
		In order to wait for the result I used pause() which pauses the prcess until a signal is received.
- Testing: Same testing as Ex4; Make sure to close any open server before, You will be required to close the server manually.
	   Run by `<test>`, Result: The last two lines simulates a client checking the stack; Requesting top & exit
	   If the results shows empty stack - Testing passed, otherwise failed. [Currently seems to work].
	   Note: Ignore trash.txt | I got error redirecting to null thus created a file and it will be deleted by `make clean`.
		
