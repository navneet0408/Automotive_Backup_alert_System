1. To compile the application, we can go to the directory and type the command:
	“make”.

2. To run the application, we will need to insert the kernel module first. To insert the kernel module we go to the directory and type:
	“insmod sensor.ko”
Then to start the application, we type:
	./led <Safe_Distance>

3. The safe distance of 5 is set by default if we do not provide any command line argument.

4. The electrical connections for the sensor are as follows:

	Vcc 	-> 	5V
	Trig	-> 	GPIO-15 (Pin-3)
	Echo	-> 	GPIO-14 (Pin-2)
	Gnd	->	Gnd

5. The electrical connections for the LED matrix are as follows:
	Vcc	->	5V
	Gnd	->	Gnd
	Din	->	Pin-11
	CS	->	Pin-10
	CLK	->	Pin-13