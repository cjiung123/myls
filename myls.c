#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>
#include <ctype.h>
#include "myQueue.h"

struct Info {
	char* name;
	char* permissions;
	ino_t inodeNum;
	nlink_t linkNum;
	off_t size;
	struct timespec modTime;
	char* groupName;
	char* userName;
};
//Queue* myInfoQueue;

int dirCount = 0;
bool option_i = false;
bool option_l = false;
bool option_R = false;

int inodeNumSpec = 0;
int linkNumSpec = 0;
int userNameSpec = 0;
int groupNameSpec = 0;
int sizeSpec = 0;

void listFiles(Queue* fileQueue);
Queue* listDirectories(Queue* dirQueue, Queue** myInfoQueueRef);
void QueueMergeSort(Queue** q);
void MergeSort(Node** headRef);
Node* Merge(Node* left, Node* right);
void SplitSubLists(Node* src, Node** leftRef, Node** rightRef);
void listDirByRecursion(Queue* dirQueue);
void printInodeNum(struct Info* info);
void printLongList(struct Info* info);
char* setPermissions(mode_t mode);
int main(int argc, char *argv[]) {

	Queue* optionsQueue = createQueue();
	Queue* fileListQueue = createQueue();

	if(argc >= 2) {
		for(int i=1; i < argc; i++) {
			if(argv[i][0] == '-') {
				if(strlen(argv[i]) == 2) {
				Enqueue(optionsQueue, &argv[i][1]);
				}
				else if(strlen(argv[i]) > 2) {
					for(int j=1; j < strlen(argv[i]); j++) {
						Enqueue(optionsQueue, &argv[i][j]);
					}
				}
				else {
					Enqueue(fileListQueue, argv[i]);
				}
			}
			else {
				Enqueue(fileListQueue,argv[i]);
			}
		}
	}

	char* option;
	while((option = Dequeue(optionsQueue)) != NULL) {
		switch(*(char*)option) {
			case 'i':
				option_i = true;
				break;
			case 'l':
				option_l = true;
				break;
			case 'R':
				option_R = true;
				break;
			default:
				fprintf(stderr, "myls: invalid option -- '%c'\n", *option);
				return 1;
		}
	}
	//printf("i: %d, l: %d, R: %d\n", option_i, option_l, option_R);
	char* fileList;
	Queue* goodFileQueue;
	Queue* goodDirQueue;
	goodFileQueue = createQueue();
	goodDirQueue = createQueue();
	if(isEmpty(fileListQueue)) {
		struct Info* info = (struct Info*) malloc(sizeof(struct Info));
		info->name = ".";
		Enqueue(goodDirQueue, info);
	}
	else {
		while((fileList = Dequeue(fileListQueue)) != NULL) {
			//printf("%s\n", fileList);
			struct stat sb;
			struct Info* info = (struct Info*) malloc(sizeof(struct Info));
			if(lstat(fileList, &sb) == -1) {
				fprintf(stderr, "myls: cannot access '%s': No such file or directory\n", fileList);
				continue;
			}
			if((sb.st_mode & S_IFMT) == S_IFDIR) {
				info->name = fileList;
				Enqueue(goodDirQueue, info);
				dirCount++;
				//printf("Dir: %s\n", fileList);
			}
			else if((sb.st_mode & S_IFMT) == S_IFLNK) {
				info->name = fileList;
				Enqueue(goodDirQueue, info);
				dirCount++;
				//printf("Link: %s\n", fileList);
			}
			else {
				struct passwd* userName = getpwuid(sb.st_uid);
				struct group* groupName = getgrgid(sb.st_gid);
				info->permissions = setPermissions(sb.st_mode);
				info->inodeNum = sb.st_ino;
				info->linkNum = sb.st_nlink;
				info->userName = strdup(userName->pw_name);
				info->groupName = strdup(groupName->gr_name);
				info->size = sb.st_size;
				info->modTime = sb.st_mtim;
				info->name = fileList;
				Enqueue(goodFileQueue, info);
				//printf("file: %s\n", fileList);
			}
		}
	}
	if(!isEmpty(goodFileQueue)) {
		listFiles(goodFileQueue);
		//add empty line if there are directories
		if(!isEmpty(goodDirQueue)) {
			printf("\n");
		}
	}
		listDirByRecursion(goodDirQueue);
	return 0;
}

void printInodeNum(struct Info* info) {
	printf("%*ju ", inodeNumSpec, info->inodeNum);
}

void printLongList(struct Info* info) {
	char month[10];
	char time[10];
	printf("%10s ", info->permissions);
	printf("%*ju ", linkNumSpec, (uintmax_t)info->linkNum);
	printf("%-*s ", userNameSpec, info->userName);
	printf("%-*s ", groupNameSpec, info->groupName);
	printf("%*ju ", sizeSpec, (uintmax_t)info->size);
	struct tm *t = localtime((time_t *) &info->modTime);
	strftime(month, 10, "%b", t);
	strftime(time, 10, "%H:%M", t);
	printf("%s %2d %4d %s ", month, t->tm_mday, t->tm_year+1900, time);
}

void listDirByRecursion(Queue* dirQueue) {

	Queue* childDir;
	Queue* myInfoQueue = createQueue();

	while(!isEmpty(dirQueue)) {
		childDir = listDirectories(dirQueue, &myInfoQueue);
		QueueMergeSort(&myInfoQueue);
		struct Info* info;
		while((info = Dequeue(myInfoQueue)) != NULL) {
			if(option_i) {
				printInodeNum(info);
			}
			if(option_l) {
				printLongList(info);
			}
			printf("%s\n", info->name);
		}

		if(option_R) {
			if(!isEmpty(childDir)) {
				printf("\n");
				listDirByRecursion(childDir);
			}
		}
		if(!isEmpty(dirQueue)) {
			printf("\n");
		}
	}
}

void listFiles(Queue* fileQueue) {
	if(!isEmpty(fileQueue)) {
		QueueMergeSort(&fileQueue);
	}

	struct Info* fileInfo;
	while((fileInfo = Dequeue(fileQueue)) != NULL) {
		if(option_i) {
			printInodeNum(fileInfo);
		}
		if(option_l) {
			printLongList(fileInfo);
		}
		printf("%s\n", fileInfo->name);
	}

}

char* setPermissions(mode_t mode) {
	char* permArr = malloc(sizeof(char) * 11);
	if((mode & S_IFMT) == S_IFDIR) {
		permArr[0] = 'd';
	}
	else if((mode & S_IFMT) == S_IFLNK) {
		permArr[0] = 'l';
	}
	else {
		permArr[0] = '-';
	}
	permArr[1] = (mode & ~S_IFMT & S_IRUSR) ? 'r' : '-';
	permArr[2] = (mode & ~S_IFMT & S_IWUSR) ? 'w' : '-';
	permArr[3] = (mode & ~S_IFMT & S_IXUSR) ? 'x' : '-';
	permArr[4] = (mode & ~S_IFMT & S_IRGRP) ? 'r' : '-';
	permArr[5] = (mode & ~S_IFMT & S_IWGRP) ? 'w' : '-';
	permArr[6] = (mode & ~S_IFMT & S_IXGRP) ? 'x' : '-';
	permArr[7] = (mode & ~S_IFMT & S_IROTH) ? 'r' : '-';
	permArr[8] = (mode & ~S_IFMT & S_IWOTH) ? 'w' : '-';
	permArr[9] = (mode & ~S_IFMT & S_IXOTH) ? 'x' : '-';
	permArr[10] = '\0';
	//printf("permArr: %s\n", permArr);
	return permArr;

}
void defineSpecifier(struct Info* info) {
	char inodeNumStr[30];
	char linkNumStr[10];
	char sizeStr[30];
	sprintf(sizeStr, "%ju", info->size);
	sizeSpec = (strlen(sizeStr) > sizeSpec) ? strlen(sizeStr) : sizeSpec;
	sprintf(inodeNumStr, "%ju", info->inodeNum);
	inodeNumSpec = (strlen(inodeNumStr) > inodeNumSpec) ? strlen(inodeNumStr) : inodeNumSpec;
	sprintf(linkNumStr, "%ju", info->linkNum);
	linkNumSpec = (strlen(linkNumStr) > linkNumSpec) ? strlen(linkNumStr) : linkNumSpec;
	userNameSpec = (strlen(info->userName) > userNameSpec) ? strlen(info->userName) : userNameSpec;
	groupNameSpec = (strlen(info->groupName) > groupNameSpec) ? strlen(info->groupName) : groupNameSpec;
}

Queue* listDirectories(Queue* dirQueue, Queue** myInfoQueueRef) {
	char *dirName;
	DIR* dir;
	struct dirent* entity;
	Queue* childDir = createQueue();
	char path[100];
	struct stat sb;
	struct Info* dirInfo;

	inodeNumSpec = 0;
	linkNumSpec = 0;
	userNameSpec = 0;
	groupNameSpec = 0;
	sizeSpec = 0;

	QueueMergeSort(&dirQueue);
	if((dirInfo = Dequeue(dirQueue)) != NULL) {
		if(option_R) {
			printf("%s:\n", dirInfo->name);
		}
		else {
			if(dirCount >= 2) {
				printf("%s:\n", dirInfo->name);
			}
		}
		dirName = dirInfo->name;
		dir = opendir(dirName);
		entity = readdir(dir);
		while(entity != NULL) {
			if(strncmp(entity->d_name, ".", 1) && strcmp(entity->d_name, "..")) {
				strcpy(path, dirName);
				if(path[strlen(path)-1] != '/') {
					strcat(path, "/");
				}
				strcat(path, entity->d_name);
				//printf("test: %s\n", test);
				lstat(path, &sb);

				struct Info* myInfo = (struct Info*) malloc(sizeof(struct Info));
				struct passwd *userName = getpwuid(sb.st_uid);
				struct group *groupName = getgrgid(sb.st_gid);
				char* permissions = setPermissions(sb.st_mode);
				myInfo->permissions = permissions;
				myInfo->inodeNum = sb.st_ino;
				myInfo->linkNum = sb.st_nlink;
				myInfo->modTime = sb.st_mtim;
				myInfo->name = entity->d_name;
				myInfo->size = sb.st_size;
				myInfo->userName = strdup(userName->pw_name);
				myInfo->groupName = strdup(groupName->gr_name);
				defineSpecifier(myInfo);

				Enqueue(*myInfoQueueRef, myInfo);
				//printf("%s\n", myInfo->name);
				if((sb.st_mode & S_IFMT) == S_IFDIR) {
					struct Info* childDirInfo = (struct Info*) malloc(sizeof(struct Info));
					childDirInfo->name = strdup(path);
					Enqueue(childDir, childDirInfo);
				}
			}
			entity = readdir(dir);
			bzero(path, 100);
		}
	}
	return childDir;
}
void QueueMergeSort(Queue** q) {
	if((*q)->front == NULL) return;
	MergeSort(&(*q)->front);
	Node* rear = (*q)->front;
	while(rear->next != NULL) {
		rear = rear->next;
	}
	(*q)->rear = rear;
}
void MergeSort(Node** headRef) {
	Node* head = *headRef;
	Node* left;
	Node* right;

	if((head == NULL) || (head->next == NULL)) {
		return;
	}
	SplitSubLists(head, &left, &right);

	MergeSort(&left);
	MergeSort(&right);

	*headRef = Merge(left, right);
}

void SplitSubLists(Node* src, Node** leftRef, Node** rightRef) {
	int size = 0;
	int subSize = 0;
	Node* temp = src;

	while(temp != NULL) {
		size++;
		temp = temp->next;
	}
	subSize = (size % 2) ? size / 2 + 1 : size / 2;

	temp = src;
	for(int i=0; i < subSize-1; i++) {
		temp = temp->next;
	}

	*leftRef = src;
	*rightRef = temp->next;
	temp->next = NULL;
}

bool lexicographicalCompare(char* a, char* b) {
	//printf("a: %s\n", a);
	//printf("b: %s\n", b);
	int j = 0;
	int minLen = (strlen(a) <= strlen(b)) ? strlen(a) : strlen(b);
	for(int i=0; i<minLen; i++) {
		if(a[i] == b[j]) {
			j+=1;
			continue;
		}
		if(a[i] == '.') {
			if(i+1 < strlen(a)) {
				i += 1;
			}
		}
		if(b[j] == '.') {
			if(j+1 < strlen(b)) {
				j += 1;
			}
		}
		if(isalpha(a[i]) && isalpha(b[j])) {
			bool isA = (a[i] >= 65 && a[i] <= 90) ? true : false;
			bool isB = (b[j] >= 65 && b[j] <= 90) ? true : false;
			if(isA && isB) {
				if(a[i] + 32 < b[j] + 32) {
					return true;
				}
				else if(a[i] + 32 > b[j] + 32){
					return false;
				}
			}
			else if(isA && !isB) {
				if(a[i] + 32 < b[j]) {
					return true;
				}
				else if(a[i] + 32 > b[j]) {
					return false;
				}
			}
			else if(!isA && isB) {
				if(a[i] < b[j] + 32) {
					return true;
				}
				else if(a[i] > b[j] + 32) {
					return false;
				}
			}
			else {//a and b both digit
				if(a[i] < b[j]) {
					return true;
				}
				else if(a[i] > b[j]) {
					 return false;
				}
			}
		}
		else if(isalpha(a[i]) && !isalpha(b[j])) {
			return false;
		}
		else if(!isalpha(a[i]) && isalpha(b[j])) {
			return true;
		}
		else {
			if(a[i] < b[j]) {
				return true;
			}
			else if(a[i] > b[j]){
				return false;
			}
		}
		j+=1;
	}

	if(strlen(a) == minLen && strlen(a) != strlen(b)) {
		return true;
	}
	else if(strlen(b) == minLen && strlen(a) != strlen(b)) {
		return false;
	}
	else {
		j = 0;
		for(int i=0; i<minLen; i++) {
			//distinguish between capital letters
			if(a[i] < b[j]) {
				return false;
			}
			else if(a[i] > b[j]){
				return true;
			}
			j+=1;
		}
	}
	return NULL;
}
Node* Merge(Node* left, Node* right) {
	Node* merged = NULL;
	if(left == NULL) {
		return right;
	}
	else if(right == NULL) {
		return left;
	}
	struct Info* leftInfo = (struct Info*)left->data;
	//printf("left->name: %s\n", leftInfo->name);
	struct Info* rightInfo = (struct Info*)right->data;
	//printf("right->name: %s\n", rightInfo->name);
		if(lexicographicalCompare(leftInfo->name, rightInfo->name)) {
			merged = left;
			merged->next =Merge(left->next, right);
		}
		else {
			merged = right;
			merged->next = Merge(left, right->next);
		}
	return merged;


	return NULL;
}
