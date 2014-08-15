/* FUSE:Filesystem in Userspace Copyright(C) 2001 - 2005 Miklos Szeredi < miklos @ szeredi.hu > Changed:Added fuse_opt.h support to show cmdline - option passing(ingenious) / code clean(rigid)
     This program can be distributed under the terms of the GNU GPL.See the file COPYING. * /
     */

#define FUSE_USE_VERSION 25
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <time.h>
#include <utime.h>
#include <fuse.h>
#include <fuse/fuse_opt.h>

char pinfo_string[1024];
int tlen;

int 	redo_pak = 1;
char	*pakptr;


struct file_list_s {
	struct 	file_list_s	*next;
	struct 	file_list_s	*previous;
	struct 	path_s		*dir;
	char			*name;
	char			*data;
	char			*entry_name;
	int			length;

};

struct path_list_s {
	struct path_s		*path;
	struct path_list_s	*next;
	struct path_list_s	*previous;
};

struct path_s	{
	char		*name;
	struct path_s	*parent;
	struct path_list_s	*entries;
	struct file_list_s	*files;
};


struct pak_header_s {
	char	id[4];
	int	dirofs;
	int	dirlen;
};

struct pak_file_s {
	char	name[56];
	int	pos;
	int	len;
};

struct pak_s {
	char	*filename;
	char	*buffer;		// complete pak loaded into memory
	int	num_files;
	int	pak_size;
	// all of those point to buf
	struct pak_header_s	*header;
	struct pak_file_s	*files;
	struct path_s		*paths;


};

struct path_s *Find_Path(struct path_s *p, char *name);
struct file_list_s *Find_File(struct path_s *p, char *name);
void	Get_Entry_From_Tree	(char *entry, struct path_s *root, struct path_s **path, struct file_list_s **file);
int	Add_Entry_In_Tree	(char *entry, struct path_s *root, struct path_s **path, struct file_list_s **file);
int	Remove_File_Entry_From_Tree (char *entry, struct path_s *p);
int	Remove_Path_Entry_From_Tree (char *entry, struct path_s *p);
char	*Create_Pak (struct pak_s *p, int *size);
void Path_Get_File_Count_Size(struct path_s *pf, int *size, int *filecount);
void Pak_Get_Size_File_Count(struct pak_s *pack, int *size, int *filecount);



struct pak_s	*pf;

void Del_Pak (struct pak_s *p)
{
	if (p == NULL)
		return;
	if (p->buffer)
		free(p->buffer);
	if (p->filename)
		free(p->filename);
	free(p);

}

struct pak_s	*Open_Pak (char *filename)
{

	struct pak_s	*pack;
	FILE *f;
	int file_size;

	/* Init */
	f = fopen(filename, "rb");

	if (f == NULL)
	{
		printf("Could not open %s\n", filename);
		return NULL;
	}

	fseek(f,0,SEEK_END);
	file_size = ftell(f);
	printf("filesize: %i\n", file_size);
	fseek(f,0,SEEK_SET);

	pack = malloc(sizeof(struct pak_s));
	pack->filename = strdup(filename);

	pack->buffer =  malloc(file_size);
	fread(pack->buffer, file_size, 1, f);
	fclose(f);

	pack->header	= (struct pak_header_s *)pack->buffer;

	if (strncmp(pack->header->id, "PACK", 4))
	{
		printf("%s is not a pak file\n", filename);
		return NULL;
	}


	pack->files	= (struct pak_file_s *)(pack->buffer + pack->header->dirofs);

	pack->num_files = pack->header->dirlen / sizeof (struct pak_file_s);
	pack->pak_size	= file_size;



	return pack;
}





     static const char *pak_str = "Hello World!\n";
     static const char *pak_path = ".pakinfo";
     static const char *pak_file = ".pakfile";

/** options for fuse_opt.h */
     struct options
     {
	     char *pf;
	     char *bar;
	     int baz;
	     double quux;
	     int new;
     } options;

/** macro to define options */
#define HELLOFS_OPT_KEY(t, p, v) { t, offsetof(struct options, p), v }

/** keys for FUSE_OPT_ options */
     enum
     {
	     KEY_VERSION,
	     KEY_HELP,
     };

     static struct fuse_opt pak_opts[] = {
	     HELLOFS_OPT_KEY("-pf %s", pf, 0),
	     HELLOFS_OPT_KEY("bar=%s", bar, 0),
	     HELLOFS_OPT_KEY("baz", baz, 1),
	     HELLOFS_OPT_KEY("-n", new, 1),

	     // #define FUSE_OPT_KEY(templ, key) { templ, -1U, key }
	     FUSE_OPT_KEY("-V", KEY_VERSION),
	     FUSE_OPT_KEY("--version", KEY_VERSION),
	     FUSE_OPT_KEY("-h", KEY_HELP),
	     FUSE_OPT_KEY("--help", KEY_HELP),
	     FUSE_OPT_END
     };


static int pak_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	int i;
	struct pak_file_s *f = NULL;
	struct path_s *p;
	struct file_list_s *ff = NULL;
	int s =0;
	int fc=0;

	printf("getattr: %s\n", path);

	memset(stbuf, 0, sizeof(struct stat));
	/*if (strcmp(path, "/") == 0)*/
	if (path[0] == '/' && strlen(path)==1)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return res;
	}

	if (strstr(path, pak_path))
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = 1024;
		return res;
	}

	if (strstr(path, pak_file))
	{
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		Pak_Get_Size_File_Count(pf, &s, &fc);
		stbuf->st_size = s;
		return res;
	}




	Get_Entry_From_Tree(path, pf->paths, &p, &ff);


	if (ff)
	{
		stbuf->st_mode = S_IFREG | 0666;
		stbuf->st_nlink = 1;
		stbuf->st_size = ff->length;
		return res;
	}

	if (p)
	{
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return res;
	}

	return -ENOENT;
}


static int pak_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	(void)offset;
	(void)fi;
	int i;
	struct path_list_s *pl;
	struct file_list_s *f;
	struct path_s *p;
	char *s,*s1;

	printf("readdir: %s\n", path);
	/*if (strcmp(path, "/") != 0)
		return -ENOENT;
		*/

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (path[0] == '/' && strlen(path) == 1)
	{
		filler(buf, pak_path , NULL, 0);
		filler(buf, pak_file , NULL, 0);
	}

	
	Get_Entry_From_Tree(path, pf->paths, &p, &f);

	if (f)
	{
		printf("readdir on a file ?\n");
		return;
	}

	if (p)
	{
		pl = p->entries;
		while (pl)
		{
			filler(buf, pl->path->name, NULL, 0);
			pl = pl->next;
		}

		f = p->files;
		while (f)
		{
			filler(buf, f->name, NULL, 0);
			f = f->next;
		}
	}

	return 0;
}

static int pak_open(const char *path, struct fuse_file_info *fi)
{

	printf("open: %s\n", path);
	/*
	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;
	*/
	return 0;
}


static int pak_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	size_t len;
	int i,filecount,psize;
	struct pak_file_s *f;
	struct file_list_s *ff;
	int found;
	char *lpf;
	char	pak_info_string[1024];

	(void)fi;

	printf("read: %s\n", path);

	/* special case for pak info */
	if (strcmp(path+1, pak_path) == 0)
	{
		filecount = 0;
		psize = 0;
		Pak_Get_Size_File_Count(pf, &psize, &filecount);
		snprintf(pak_info_string, 1024, "mounted filename: %s\nfilesize: %i\nnumber of files: %i\n", pf->filename, psize, filecount);
		len = strlen(pak_info_string);

		if (offset < len)
		{
			if (offset + size > len)
				size = len - offset;

			memcpy(buf, pak_info_string + offset, size);
		}
		else
			size = 0;

		return size;
	}

	if (strstr(path, ".pakfile"))
	{

		if (redo_pak)
		{
			if (pakptr)
				free(pakptr);
			pakptr = Create_Pak(pf, &len);
			tlen = len;
			lpf = pakptr;
			redo_pak = 0;

		}
		else
		{
			lpf = pakptr;
			len = tlen;
			
		}

		if (offset < len)
		{
			if (offset + size > len)
				size = len - offset;

			memcpy(buf, pakptr + offset, size);
		}
		else
			size = 0;


		return size;
	}

	Get_Entry_From_Tree(path, pf->paths, NULL, &ff);

	if (!ff)
		return -ENOENT;


	if (offset < ff->length)
	{
		if (offset + size > ff->length)
			size = ff->length - offset;

		memcpy(buf, ff->data + offset, size);
	}
	else
		size = 0;






	return size;
}

char *get_filename(char *path);

static int pak_mkdir(const char *path, mode_t mode)
{
	int res;
	struct path_s *p;

	printf("mkdir: %s\n",path);
	if (strlen(path) >55)
		return -ENOENT;
	p = malloc(sizeof(struct path_s));
	memset(p, 0, (sizeof(struct path_s)));
	p->name = get_filename(path);
	Add_Entry_In_Tree(path, pf->paths, &p, NULL);


	return 0;
}


static int pak_unlink(const char *path)
{
	printf("unlink: %s\n", path);
	if (strstr(path,".pak"))
		return 0;

	redo_pak = 1;
	return Remove_File_Entry_From_Tree(path, pf->paths);
}

static int pak_rmdir(const char *path)
{
	return -ENOENT;
	printf("rmdir: %s\n", path);
	return Remove_Path_Entry_From_Tree(path, pf->paths);
}


static int pak_write (const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{

	struct file_list_s *f;
	char *fbuf;

	printf("write: %s\n",path);


	Get_Entry_From_Tree(path, pf->paths, NULL, &f);

	if (f == NULL)
		return -ENOENT;

	fbuf = malloc(f->length + size);
	memcpy(fbuf, f->data, f->length);
	memcpy(fbuf+f->length, buf, size);
	free (f->data);
	f->data = fbuf;
	f->length = f->length + size;



	return size;


}

static int pak_mknod(const char *path, mode_t mode, dev_t rdev)
{

	char *fname;
	struct file_list_s *ff;
	int ret;

	fname = get_filename(path);
	ff = malloc(sizeof(struct file_list_s));
	memset(ff, 0, (sizeof(struct file_list_s)));
	ff->name = fname;
	ret =  Add_Entry_In_Tree(path, pf->paths, NULL, &ff);
	return ret;



}

static int pak_utime(const char *path, const struct utimbuf *buf)
{


	return 0;
}

/** This tells FUSE how to do every operation */
static struct fuse_operations pak_oper = {
	.getattr	= pak_getattr,
	.readdir	= pak_readdir,
	.open		= pak_open,
	.read		= pak_read,
	.mkdir		= pak_mkdir,
	.unlink		= pak_unlink,
	.rmdir		= pak_rmdir,
	.write		= pak_write,
	.mknod		= pak_mknod,
	.utime		= pak_utime,
};



void Paths_Get(struct pak_s *pack);

int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);



	/* clear structure that holds our options */
	memset(&options, 0, sizeof(struct options));

	if (fuse_opt_parse(&args, &options, pak_opts, NULL) == -1)
			/** error parsing options */
		return -1;


	if (options.pf)
	{
		printf("test\n");
		printf("%s\n", options.pf);
		pf = Open_Pak(options.pf);
	}

	if (options.new)
	{
		pf = malloc(sizeof(struct pak_s));
		memset(pf,0,sizeof(struct pak_s));
		pf->filename = strdup("new pack");
	}


	if (pf == NULL)
	{
		printf("ERROR !\n");
		return 2;
	}

	Paths_Get(pf);

	ret = fuse_main(args.argc, args.argv, &pak_oper);

	if (ret)
		printf("\n");

		/** free arguments */
	fuse_opt_free_args(&args);

	return ret;
}
