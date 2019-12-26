Csc 360
Assignment: 2
Dallas Brooks
V00868024

To compile program:
	make
To run program:
	./ACS <file>.txt

Input file must be in the form:
	1. First character specifies the unique ID of customers
	2. A colon(:) immediately follows the unique number of the customer
	3. Immediately following is an integer equal to either 1 (indicating the customer belongs to business class) or 0 (indicating the customer belongs to economy class)
	4. A comma(,) immediately follows the previous number
	5. Immediately following is an integer that indicates the arrival time of the customer
	6. A comma(,) immediately follows the previous number
	7. Immediately following is an integer that indicate the service time of the customer
	8. a newline(\n) ends a line

	Example input file:
		7
		1:0,2,60
		2:0,4,70
		3:0,5,50
		4:1,7,30
		5:1,7,40
		6:1,8,50
		7:0,10,30