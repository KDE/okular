/* 
 detects the path of the started executable from argv[0] and the enviroment-
 variable $PATH 
 Copyright (c) MUFTI (Dipl. Phys. J. Scheurich (scheurich@rus.uni-stuttgart.de))

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You could have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Fixed bug with relative paths in $PATH and on the commandline; also
 * fixed some other bugs concerning '//' as directory seperator, spaces
 * in $PATH and a misplaced dump_on_memoryleak.
 * This file is a mess and I'm sure I've left some bugs in. Somebody should
 * consider rewriting it. But it's late and I'd like to go to bed now :-)
 * 6.2.95, Martin Buck <martin.buck@student.uni-ulm.de>
 *
 * Some fixes for brain-damaged Ultrix-systems
 * 21.2.95, Martin Buck <martin.buck@student.uni-ulm.de>
 */


#include "c-auto.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <kpathsea/c-dir.h>
/* the following is in sysv's stat.h, but not in bsd4.3's */
#ifndef S_IXUSR
#define S_IXUSR        0000100
#endif
#ifndef S_IXGRP
#define S_IXGRP        0000010
#endif
#ifndef S_IXOTH
#define S_IXOTH        0000001
#endif


#include <unistd.h>
#include <kpathsea/config.h>
#include <kpathsea/c-pathmx.h>

#ifndef TRUE
#define TRUE -1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define CONSTLINK      1
#define CONSTFILE      2
#define CONSTDIRECTORY 3 
#define CONSTSPECIAL   4

/* 
 * Avoid name conflicts with other routines of a user  
 * only the "main"-routine(selfdir) has a common name
 */
/*
 * I always thought declaring functions as static would be the solution
 * to this problem... Am I missing something?
 */

#define dump_on_memoryleak _dump_on_memoryleak_
#define existinode         _existinode_
#define identifyinode      _identifyinode_
#define testexecutable     _testexecutable_
#define inodeoflink        _inodeoflink_
#define getdir             _getdir_
#define getlink            _getlink_
#define absolutepath       _absolutepath_
#define searchfile         _searchfile_

void dump_on_memoryleak(char* buff);

/* 
 * search if the inode pointed by directory and file exist 
 * give back inode on success, else return NULL
 */

char* existinode(char* directory,char* file)
   {
   int result;
   struct stat buf;
   char* inode;
   char* nothing;

   inode=(char*)malloc((strlen(directory)+strlen(file)+2)*sizeof(char));
   dump_on_memoryleak(inode);
   nothing=strcpy(inode,directory);
   if (directory[0]!=(char)0)
      strcat(inode,"/");
   strcat(inode,file);
   result=stat(inode,&buf);
   if (result==0)
      return(inode);
   free(inode); 
   return(NULL);
   }  

/*
 * identify inode, if it is link, directory, file or special 
 */

int identifyinode(char* inode)
   {
   int result;
   struct stat buf;
/* 
   note, that it's important to use here lstat instead of stat under POSIX,
   cause stat can't get back the st_mode of the link itself (it gets the
   st_mode of the pointed inode)
 */
   result=lstat(inode,&buf);
   if (result==-1)
      return(0);   
   if      (((buf.st_mode) & S_IFLNK)==S_IFLNK) result=CONSTLINK;
   else if (((buf.st_mode) & S_IFDIR)==S_IFDIR) result=CONSTDIRECTORY;
   else if (((buf.st_mode) & S_IFREG)==S_IFREG) result=CONSTFILE;
   else                                         result=CONSTSPECIAL;
   return(result);
   }

/*
 * test if the file is executable, if yes, return inode, if no return NULL
 */

char* testexecutable(char* inode)
   {
   int result;
   struct stat buf;

   result=stat(inode,&buf);
   if (result<0)
      return(NULL);      
   result=-1;
   if      (((buf.st_mode) & S_IXUSR) != 0) result=0;
   else if (((buf.st_mode) & S_IXGRP) != 0) result=0;
   else if (((buf.st_mode) & S_IXOTH) != 0) result=0;
   if (result<0)
      return(NULL);   
   return(inode);
   }

/*
 * search the inode of a filelink
 */

char* inodeoflink(char* inode)
   {
   int result;
   int i;
   size_t buflen=MAXNAMLEN;
   char* buf;

   buf=(char*)malloc((MAXNAMLEN+1)*sizeof(char));
   dump_on_memoryleak(buf);
   for (i=0;i<=MAXNAMLEN;i=i+1)
      buf[i]=(char)0;
   result=readlink(inode,buf,buflen);
   buf[buflen]=(char)0;
   if (result<0)
      return(NULL);   
   return(buf);
   }

/*
 *  make a coredump, if there is no more memory 
 */

void dump_on_memoryleak(char* buff)
   {
   int i;
   if (buff==NULL)
      {
      perror(" in self.c ");
      printf("can't malloc, make coredump\n");
      i=0;      
      i=1/i;
      /* avoid to optimise the former line away ... */
      printf("%d\n",i);
      exit(ENOMEM);
      }
   }

/*
 * give back the directory of a file
 */

char* getdir(char* file)
   {
   int i;
   if (file==NULL) return(file);
   for (i=strlen(file)-1;i>=0;i=i-1)
      if (file[i]=='/')
         { 
         while (i >= 0 && file[i] == '/') {
           file[i+1]=(char)0; 
           i--;
         }
         i = 0;
         break;
         }
   if (i<0)
      return(NULL);
   return(file);
   }

/* 
 * get a proper path of the inode of a FILElink
 */

char* getlink(char* inode)
   {
   char* inodepath;
   char* temp;
   char* returninode;

   returninode=inodeoflink(inode);
   if (returninode==NULL)
      {
      return(NULL);
      free(inode);
      }   
/* relative links need extra code ! */
   if (returninode[0]!='/')
      {
      inodepath=getdir(inode); 
      temp=malloc(strlen(inodepath)+strlen(returninode)+1);
      dump_on_memoryleak(temp);
      strcpy(temp,inodepath); 
      while ((returninode[0] == '.') && (returninode[1] == '.') &&
        (returninode[2] == '/') && strlen(temp)) {
          returninode += 3;
          while (*returninode == '/')	/* Multiple slashes are valid */
            returninode++;		/* directory-seperators ! */
          temp[strlen(temp)-1]='\0';
          getdir(temp);
      }
      strcat(temp,returninode);
      returninode=temp;
      }
   free(inode);
   return(returninode);
   }

/*
 * Make an absolute path from a relative one. We assume that nobody
 * chdir'ed up to now.
 */

#ifndef HAVE_GETCWD
#define getcwd(a,b) getwd(a)
#endif

char *
absolutepath(char* path) {
  char* apath;

  if (path[0] == '/') {
    apath = xstrdup(path);
    return(apath);
  } else {
    while (path[0] == '.' && path[1] == '/') {
      path += 2;
      while (*path == '/')
        path++;
    }
    apath = (char*)malloc((PATH_MAX + strlen(path) + 2) * sizeof(char));
    dump_on_memoryleak(apath);
    apath = getcwd(apath, PATH_MAX);
    while (path[0] == '.' && path[1] == '.' && path[2] == '/' && strlen(apath)) {
      path += 3;
      while (*path == '/')
        path++;
      apath[strlen(apath) - 1] = '\0';
      getdir(apath);
    }
    if (strlen(apath) && apath[strlen(apath) - 1] != '/')
      strcat(apath, "/");
    strcat(apath, path);
    return(apath);
  }
}

/* 
 * look if the inode pointed by directory and file is a true file 
 * if the inode is a filelink, search the true file
 * if the inode is a directory or a special file, return error (NULL)
 * on success (file found) and it's executable return the path
 */

char* searchfile(char* inode)
   {
   while (identifyinode(inode)==CONSTLINK)
      inode=getlink(inode);
   if (inode!=NULL) 
      if (identifyinode(inode)!=CONSTFILE) 
         {
         free(inode);
         return(NULL);
         }
   inode=testexecutable(inode);
   return(inode);
   }

/* 
 * detects the path of the called binary from argv[0] and the enviroment-
 * variable $PATH (the path includes the last slash of the file) 
 */

char *selfdir(char* argv0)
   {
   char* inode;
   char* temp;
   char* dollarpath;
   int pathlen;
   int i;
   char* target;
   char* apath;

/* if argv0 contains a path don't search any more */ 
   for (i=0;i<=strlen(argv0);i++)
      if (argv0[i]=='/') 
         {
         target=(char*)malloc((strlen(argv0)+1)*sizeof(char));
         dump_on_memoryleak(target);	/* Checking first seems to be a good idea */
         strcpy(target,argv0);
         apath = absolutepath(target);
         free(target);
         return(getdir(searchfile(apath)));
         }
   temp=getenv("PATH");
   pathlen=strlen(temp);
   dollarpath=(char*)malloc((pathlen+1)*sizeof(char));
   dump_on_memoryleak(dollarpath);
   strcpy(dollarpath,temp);
/* strip whitespaces and ':' from path-variable */
   for (i=0;i<pathlen;i++) 
      if ((*(dollarpath+i)==':')) /* || (*(dollarpath+i)<=' ')) Why not have spaces in $PATH ? */
         *(dollarpath+i)=(char)0;
/* walk through all paths */
   i=0;
   while (i<pathlen)
      if (*(dollarpath+i)!=(char)0)
         {
         inode=existinode(dollarpath+i,argv0);  
         if (inode!=NULL)  
            {          
            apath = absolutepath(inode);
            free(inode);
            target=getdir(searchfile(apath));
            if (target!=NULL)
               {
               free(dollarpath);
               return(target);
               }
            }
         i=i+strlen(dollarpath+i); 
         }
      else
         i=i+1;
   free(dollarpath);
   return(NULL);
   }

