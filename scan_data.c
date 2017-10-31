// File name: scan_data.c
// Author: Dmitri Roukin
// Purpose: Read data from RAM file line-by-line
// 	check sensor values according to sensor struct limits
// 	issue a warning if data out of limits
// 
//	Inputs: (optional) timestamp - if the same as in file, exit
//
// Status: Alpha 1 
// 	Should not be used in production - no boundary checks, ASCII input only
//
// Build command: w|!gcc -o scan_data.a -lm && ./scan_data.a
//
// TODO: Add boundary checks
// 	Put sensor info into config file
// 	aggregate mutltiple alerts to save space


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// sensor type with limits and name
struct sensor_type{
	int max;
	int min;
	char *name;
};

// 5 sensors at the moment
struct sensor_type sensor[5];

// set snesor limits and names
void setup_sensors();

void main(int argc, char** argv){
	FILE* fd;			//RAM file descriptor
	char* memfile = "/mnt/memory/memfile.txt";//file path
	char* line = NULL;		//line to be read from file
	int len = 0;			//defaul line lenght
	int line_len = 0;		//read line length
	int counter = 0;		//counter
	int i;				//iterator
	char c;				//input char
	char hex[4];			//placeholder for hex string
	char delim = ' ';		//sensor delimter
	
	setup_sensors();		//setup sensors now

	fd = fopen(memfile, "r");	//open RAM file for reading
	while((line_len = getline(&line,&len,fd))!=-1){
		//read all lines in file
		if(counter == 0){//first line supposed to be timestamp	
			//assume the format is: time 1234567....\n\0
			line[strlen(line)-1] = '\0';//make proper string
			long t = atol(line+5);//actual timestamp starts at 5
			//if file timestamp is equal to current time stamp = exit
			if(strcmp(argv[1],line+5) == 0) exit(0);
		}else{	//all other lines
			//read sensor data
			int sc = 0;	//sensor count
			int alert = 0;	//alert indicator
			for(i=0;i<line_len;i++){//parse line
				//skip first field
				if(line[i] == delim){//get next field
					//get sensor data
					memcpy(hex,line+i+1,3);
					hex[3] = '\0';
					int val = strtol(hex,NULL,16);//string to HEX
					//check limits
					if(val > sensor[sc].max || val < sensor[sc].min){
						//display alert on stdout
						printf("Action on %s = %x (%s)\n",sensor[sc].name,val, hex);
						//update alert status
						alert = 1;
					}
					sc++;	//update sensor counter
					if(sc > 4) break;//if at sensor limit - go to next line
				}
			}
			//if(alert) printf("%s",line);
		}
		counter++;
	}


}


void setup_sensors(){
	//setup 5 sensors with names and limits
	//res 1
	sensor[0].name = (char*)malloc(8);
	strcpy(sensor[0].name,"___res1");
	sensor[0].min = 0x100;
	sensor[0].max = 0x300;
	//res 2
	sensor[1].name = (char*)malloc(8);
	memcpy(sensor[1].name,"___res2", 7);
	sensor[1].min = 0x100;
	sensor[1].max = 0x300;

	//mag
	sensor[2].name = (char*)malloc(8);
	strcpy(sensor[2].name,"_magnet");
	sensor[2].min = 0x190;
	sensor[2].max = 0x1bf;


	//light
	sensor[3].name = (char*)malloc(8);
	strcpy(sensor[3].name,"__light");
	sensor[3].min = 0x10;
	sensor[3].max = 0x300;

	//temp
	sensor[4].name = (char*)malloc(8);
	strcpy(sensor[4].name,"___temp");
	sensor[4].min = 0x300;
	sensor[4].max = 0x395;
}
