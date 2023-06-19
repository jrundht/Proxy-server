/*
 * This file implements two functions that read XML and binary information from a buffer,
 * respectively, and return pointers to Record or NULL.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "recordFromFormat.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

Grade getGrade(char *grade){
    if(strcmp(grade, "None") == 0)          return Grade_None;
    else if(strcmp(grade, "Bachelor") == 0) return Grade_Bachelor;
    else if(strcmp(grade, "Master") == 0)   return Grade_Master;
    else if(strcmp(grade, "PhD") == 0)      return Grade_PhD;
    else                                    return -1;    
}

int getCourseCode(char *course){
    if(strcmp(course, "IN1000") ==0)        return Course_IN1000;
    else if(strcmp(course, "IN1010") ==0)   return Course_IN1010;
    else if(strcmp(course, "IN1020") ==0)   return Course_IN1020;
    else if(strcmp(course, "IN1030") ==0)   return Course_IN1030;
    else if(strcmp(course, "IN1050") ==0)   return Course_IN1050;
    else if(strcmp(course, "IN1060") ==0)   return Course_IN1060;
    else if(strcmp(course, "IN1080") ==0)   return Course_IN1080;
    else if(strcmp(course, "IN1140") ==0)   return Course_IN1140;
    else if(strcmp(course, "IN1150") ==0)   return Course_IN1150;
    else if(strcmp(course, "IN1900") ==0)   return Course_IN1900;
    else if(strcmp(course, "IN1910") ==0)   return Course_IN1910;
    else                                    return -1;
}

/* This function is used in XMLtoRecord to extract the tagValue
 * from a tag. 
 */
void getTagValue(const char *buffer, const char *tag, char *tagValue, int tagValueSize) {
    char *tagStart = strstr(buffer, tag) + strlen(tag);

    strncpy(tagValue, tagStart, tagValueSize - 1); 
    char *endOfValue = strchr(tagValue, '"');        
    if(endOfValue != NULL) *endOfValue = '\0';                     
}

/* Receives a buffer containing record(s) in xml, searches for start and end of record,
 * makes a buffer containing the current record. Extracts the value inside each tag until one whole record has been read
 *
 * Returns NULL if the record is incomplete, returns a pointer to a record otherwise
*/ 
Record* XMLtoRecord( char* buffer, int bufSize, int* bytesread ){
    Record *record = newRecord();

    char *startOfRecord = strstr(buffer + *bytesread, "<record>");
    if(!startOfRecord){ 
        deleteRecord(record);
        return NULL;
    }

    char *endOfRecord = strstr(buffer + *bytesread, "</record>");
    if(!endOfRecord){
        deleteRecord(record);
        return NULL;
    }

    int lenRecord = endOfRecord - startOfRecord + strlen("</record>");
    char oneRecord[lenRecord + 1];
    memcpy(oneRecord, startOfRecord, lenRecord);
    oneRecord[lenRecord] = '\0';
    
    char *startTag = strstr(oneRecord, "<record>");
    if(!startTag){ 
        fprintf(stderr, "Could not locate <record>\n");
        deleteRecord(record);
        return NULL;
    }
    startTag += strlen("<record>");
    
    while(startTag && startTag < oneRecord + lenRecord){  
        char tagValue[bufSize];                     

        memset(tagValue, 0, sizeof(tagValue)); 
        if(strstr(startTag, "source=")){
            getTagValue(startTag, "source=\"", tagValue, sizeof(tagValue));   
            setSource(record, tagValue[0]);
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "dest=")){
            getTagValue(startTag, "dest=\"", tagValue, sizeof(tagValue));  
            setDest(record, tagValue[0]);            
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "username=")){
            getTagValue(startTag, "username=\"", tagValue, sizeof(tagValue));
            setUsername(record, tagValue);                                  
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "id=")){
            getTagValue(startTag, "id=\"", tagValue, sizeof(tagValue));
            setId(record, atoi(tagValue));
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "group=")){
            getTagValue(startTag, "group=\"", tagValue, sizeof(tagValue));
            setGroup(record, atoi(tagValue));
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "semester=")){
            getTagValue(startTag, "semester=\"", tagValue, sizeof(tagValue));
            setSemester(record, atoi(tagValue));
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "grade=")){
            getTagValue(startTag, "grade=\"", tagValue, sizeof(tagValue));
            int grade = getGrade(tagValue);

            if(grade != -1) setGrade(record, grade);
            startTag = strstr(startTag, "/>") + 2;

        }else if(strstr(startTag, "courses")){
            char *startCourses = strstr(startTag, "<courses>") + strlen("<courses>");
            char *endCourses = strstr(startCourses, "</courses>");

            while(strstr(startCourses, "course=\"") && startCourses < endCourses){
                memset(tagValue, 0, sizeof(tagValue));
                getTagValue(startCourses, "course=\"",tagValue, sizeof(tagValue));

                int courseCode = getCourseCode(tagValue);
                if(courseCode != -1)setCourse(record, courseCode);
                startCourses = strstr(startCourses, "/>") + 2;
            }
            startTag = endCourses + strlen( "</courses>");
        }else{
            startTag++;
        }
    }
    
    *bytesread = endOfRecord - buffer + strlen("</record>") + 1; //move pointer beyond </record>
    return record;
}

/* This function is used to check if the requested bytes are actually in the buffer*/
int checkBytes(Record *record, int bufSize, int readBytes, size_t nBytes){
    if(readBytes > (bufSize - (int)nBytes)){
        deleteRecord(record);
        return -1;
    }
    return 0;
}

/* Receives a buffer containing data in binary
 * Reads the first byte containing the information of which flags as assigned, 
 * if no flags the function returns NULL. Starts reading from *bytesread, making sure to offset the reading in case of more 
 * than one record in buffer
 * 
 * Reads each flag and assign to a record 
 * The function returns a pointer to a record 
*/
Record* BinaryToRecord( char* buffer, int bufSize, int* bytesread ){ 
    Record *record = newRecord();
    if(bufSize < *bytesread){ 
        fprintf(stderr, "The bufSize is smaller than the bytes read\n");
        deleteRecord(record);
        return NULL;
    }

    int readBytes = *bytesread;
    uint8_t flag = buffer[readBytes++];
    if(flag == 0){
        fprintf(stderr, "No flags present\n"); 
        deleteRecord(record);
        return NULL;
    }
    
    if(flag & FLAG_SRC){
        if(checkBytes(record, bufSize, readBytes, 1) == -1) return NULL;
        setSource(record, buffer[readBytes++]);
    }
    if(flag & FLAG_DST){
        if(checkBytes(record, bufSize, readBytes, 1) == -1) return NULL;
        setDest(record, buffer[readBytes++]);
    }
    if(flag & FLAG_USERNAME){
        uint32_t usrLen;
        if(checkBytes(record, bufSize, readBytes, sizeof(uint32_t)) == -1) return NULL;
        
        memcpy(&usrLen, &buffer[readBytes], sizeof(uint32_t)); 
        usrLen = ntohl(usrLen);
        readBytes += sizeof(uint32_t);

        if(checkBytes(record, bufSize, readBytes, sizeof(uint32_t)) == -1) return NULL;
        
        char usr[usrLen+1];
        strncpy(usr, &buffer[readBytes], usrLen); 
        usr[usrLen] = '\0';                         

        setUsername(record, usr);
        readBytes += usrLen;
    }
    if(flag & FLAG_ID){
        uint32_t id;
        if(checkBytes(record, bufSize, readBytes, sizeof(uint32_t)) == -1) return NULL;

        memcpy(&id, &buffer[readBytes], sizeof(uint32_t));
        id = ntohl(id);

        setId(record, id);
        readBytes += sizeof(uint32_t);
    }
    if(flag & FLAG_GROUP){
        uint32_t group;
        if(checkBytes(record, bufSize, readBytes, sizeof(uint32_t)) == -1) return NULL;

        memcpy(&group, &buffer[readBytes], sizeof(uint32_t));
        group = ntohl(group);

        setGroup(record, group);
        readBytes += sizeof(uint32_t);        
    }
    if(flag & FLAG_SEMESTER){
        uint8_t semester;
        if(checkBytes(record, bufSize, readBytes, sizeof(uint8_t)) == -1) return NULL;

        memcpy(&semester, &buffer[readBytes], sizeof(uint8_t));
        
        setSemester(record, semester);
        readBytes += sizeof(uint8_t);        
    }
    if(flag & FLAG_GRADE){
        uint8_t grade;
        if(checkBytes(record, bufSize, readBytes, sizeof(uint8_t)) == -1) return NULL;

        memcpy(&grade, &buffer[readBytes], sizeof(uint8_t));

        if(grade < 4){
            setGrade(record, (Grade)grade);
        }else{
            fprintf(stderr, "Invalid grade: %d\n", grade);   
        }
        readBytes += sizeof(uint8_t);        
    }
    if(flag & FLAG_COURSES){
        uint16_t courses;
        if(checkBytes(record, bufSize, readBytes, sizeof(uint16_t)) == -1) return NULL;

        memcpy(&courses, &buffer[readBytes], sizeof(uint16_t));
        courses = ntohs(courses);

        if( courses & 1<<0 ) setCourse(record, 1<<0);
        if( courses & 1<<1 ) setCourse(record, 1<<1);
        if( courses & 1<<2 ) setCourse(record, 1<<2);
        if( courses & 1<<3 ) setCourse(record, 1<<3);
        if( courses & 1<<4 ) setCourse(record, 1<<4);
        if( courses & 1<<5 ) setCourse(record, 1<<5);
        if( courses & 1<<6 ) setCourse(record, 1<<6);
        if( courses & 1<<7 ) setCourse(record, 1<<7);
        if( courses & 1<<8 ) setCourse(record, 1<<8);
        if( courses & 1<<9 ) setCourse(record, 1<<9);
        if( courses & 1<<10) setCourse(record, 1<<10);
        readBytes += sizeof(uint16_t);
    }

    *bytesread = readBytes;
    return record;
}