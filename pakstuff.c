#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>



char **split_path ( char *path)
{
	int i;
	char *s,*r,*t;
	char **cp,**cpp;

	s = path;
	i = 0;

	if (*s=='/')
		s++;
	while ((s=strchr(s,'/')))
	{
		s++;i++;
	}

	if (i == 0)
		return NULL;



	cpp = (char **) malloc ((i+1) * sizeof(char *));

	cpp[i] = NULL;
	s = path;

	if (*s=='/')
	{
		s++;
	}
	r=s;
	cp = cpp;
	i = 0;
	while ((s=strchr(s,'/')))
	{
		s++;
		t = malloc(s-r);
		strncpy(t,r,s-r);
		t[s-r-1] = 0;
		*cp = t;
		cp++;
		r=s;
	}




	return cpp;

}

void clean_splitpath (char **sp)
{
	char **cp;
	int i =0;

	if (sp == NULL)
		return;

	cp = sp;

	while (*cp != NULL)
	{
		//free(cp);
		cp++;
	}
	free(sp);
}

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



char *get_filename ( char*path)
{
	char *s, *r;

	s = strrchr(path, '/');

	if (s == NULL)
		return strdup(path);
	s++;
	return strdup(s);

}


/* returns all the strings seperated by */
char **split_dir ( char *path, int *elements)
{
	int i;
	char *s,*r,*t;
	char **cp,**cpp;

	*elements = 0;

	if (path == NULL)
		return NULL;
	if (path[0] == '/' && path[1] == '\0')
		return NULL;

	s = path;
	i = 0;

	if (*s=='/')
		s++;
	while ((s=strchr(s,'/')))
	{
		s++;i++;
	}

	/* entry in the root path */
	if (i == 0)
	{
		i++;
		cp = cpp = (char **) calloc(i, sizeof(char *));
		s = path;
		if (*s=='/')
			s++;
		*cp = strdup(s);
		*elements = i;
		return cpp;
	}

	/* else */

	i++;
	cpp = (char **) calloc (i, sizeof(char *));

	s = path;

	if (*s=='/')
	{
		s++;
	}
	r=s;
	cp = cpp;
	while ((s=strchr(s,'/')))
	{
		s++;
		t = malloc(s-r);
		strncpy(t,r,s-r);
		t[s-r-1] = 0;
		*cp = t;
		cp++;
		r=s;
	}

	s = strrchr(path, '/');
	s++;

	*cp = strdup(s);


	*elements = i;

	return cpp;

}

void split_dir_cleanup (char **cc, int entries)
{
	char **ccp;
	int x;

	ccp = cc;
	for (x=0;x<entries;x++)
	{
		free(*ccp);
		ccp++;
	}
	free(cc);


}


int	Add_Entry_In_Tree	(char *entry, struct path_s *root, struct path_s **path, struct file_list_s **file)
{
	int	entries;
	char	**centries, **cp;
	int 	i;
	struct  path_s		*add_path,*p,*pb;
	struct  path_list_s	*pl,*plb;
	struct 	file_list_s	*f,*fp;
	int	found = 0;


	if (path == NULL && file == NULL)
		return -1;

	if (entry == NULL)
		return -1;


	if (root == NULL)
		return -1;


	centries = split_dir(entry, &entries);

	if (entries == 0)
		return -1;

	if (entries == 1)
	{
		add_path = root;
		cp = centries;
	}
	else
	{
		p = root;
		cp = centries;

		for (i=0;i<entries-1;i++)
		{
			found = 0;
				if (p->entries == NULL)
				{
					pb = p;
					p->entries = malloc(sizeof(struct path_list_s));
					memset(p->entries, 0, (sizeof(struct path_list_s)));

					p->entries->next = NULL;
					p->entries->previous = NULL;
					p->entries->path = malloc(sizeof(struct path_s));
					memset(p->entries->path, 0, (sizeof(struct path_s)));
					p->entries->path->name = strdup(*cp);
					p->entries->path->parent = p;
					p->entries->path->files = NULL;
					p->entries->path->entries = NULL;
					p = p->entries->path;


				}
				else
				{
					pl = p->entries;
					plb = pl;
					
					pb = p;
					while (pl)
					{
						if (!strcmp(pl->path->name, *cp))
						{
							p = pl->path;
							found = 1;
							break;
						}
						plb = pl;
						pl = pl->next;
					}

					if (pb == p && found == 0)
					{
						plb->next = malloc(sizeof(struct path_list_s));				
						memset(plb->next, 0, (sizeof(struct path_list_s)));
						pl = plb->next;
						pl->next = NULL;
						pl->previous = plb;
						pl->path = malloc(sizeof(struct path_s));
						memset(pl->path, 0, (sizeof(struct path_s)));
						pl->path->parent = p;
						pl->path->entries = NULL;
						pl->path->name = strdup(*cp);
						pl->path->files = NULL;
						p =  pl->path;
					}
				}
			cp++;
		}
		add_path = p;
	}

	p = add_path;

	if (file)
	if (*file)
	{
		if (p->files == NULL)
		{
			p->files = *file;
			(*file)->dir = p;
			(*file)->entry_name = strdup(entry);
			split_dir_cleanup(centries,entries);
			return 0;
		}

		f = p->files;

		while (f)
		{
			if (!strcmp(f->name, *cp))
			{
				printf("file already existed %s in path %s\n", f->name, p->name);
				split_dir_cleanup(centries,entries);
				return -1;
			}
			fp = f;
			f = f->next;
		}
		fp->next = *file;
		(*file)->next = NULL;
		(*file)->previous = fp;
		(*file)->dir = p;
		(*file)->entry_name = strdup(entry);
		split_dir_cleanup(centries,entries);
		return 0;
	}

	if (path)
	if (*path)
	{
		plb = pl = p->entries;
		while (pl)
		{
			if (!strcmp(pl->path->name, *cp))
			{
				printf("path already existed\n");
				split_dir_cleanup(centries,entries);
				return -1;
			}
			plb = pl;
			pl = pl->next;
		}

		if (plb == NULL)
		{
			p->entries = malloc(sizeof(struct path_list_s));
			memset(p->entries, 0, (sizeof(struct path_list_s)));
			pl = p->entries;
			plb = NULL;
		}
		else
		{
			plb->next = malloc(sizeof(struct path_list_s));
			memset(plb->next, 0, (sizeof(struct path_list_s)));
			pl = plb->next;
		}
		pl->next = NULL;
		pl->previous = plb;
		pl->path = (*path);
		(*path)->entries = NULL;
		(*path)->files = NULL;
		(*path)->parent = p;

	}


	split_dir_cleanup(centries,entries);
	return 0;

}





void	Get_Entry_From_Tree	(char *entry, struct path_s *root, struct path_s **path, struct file_list_s **file)
{


	int	entries;
	char	**centries, **cp;
	int 	i;
	struct  path_s		*p,*pb;
	struct  path_list_s	*pl;
	struct  file_list_s	*fl;
	int	found;

	if (path == NULL && file == NULL)
		return;

	if (path)
		*path = NULL;
	if (file)
		*file = NULL;

	if (entry == NULL)
		return;

	if (entry[0] == '/' && strlen(entry) == 1)
	{
		*path = root;
		return;
	}

	centries = split_dir(entry, &entries);

	if (entries == 0)
	{

		*path = root;
		return;
	}

	p = root;
	cp = centries;

	for (i=0 ; i<entries ; i++)
	{
		found = 0;
		if (i+1 == entries && file)
		{
			fl = p->files;
			while (fl)
			{
				if (!strcmp(fl->name, *cp))
				{
					*file = fl;
					break;
				}
				fl=fl->next;
			}
		}
		pl = p->entries;

		while (pl)
		{
			found = 0;
			if (!strcmp(pl->path->name, *cp))
			{
				found = 1;
				p = pl->path;
				break;
			}

			pl = pl->next;
		}

		cp++;
	}
	if (path && found)
	{
		*path = p;
	}

	split_dir_cleanup(centries, entries);

}

int	Remove_Path_Entry_From_Tree (char *entry, struct path_s *p)
{
	struct path_s *r;
	struct path_list_s *pl, *plb;

	Get_Entry_From_Tree(entry, p, &r, NULL);

	if (r)
	{
		if (r->entries)
			return -1;
		if (r->files)
			return -1;

		pl = plb = r->parent->entries;
		while (pl)
		{
			plb = pl;

			if (r == pl->path)
				break;
			pl = pl->next;
		}
		if (pl->previous == NULL)
		{
			if (pl->next == NULL)
			{
				p->parent->entries = NULL;
				free(pl);
			}
			else
			{
				p->parent->entries = pl->next;
				pl->next->previous = NULL;
				free(pl);
			}
		}
		else
		{
			if (pl->next == NULL)
			{
				pl->previous->next = NULL;
				free(pl);
			}
			else
			{	pl->previous->next = pl->next;
				pl->next->previous = pl->previous;
				free(pl);
			}
		}
		free(r->name);
		free(r);
		return 0;



	}

	return -1;

}

int	Remove_File_Entry_From_Tree (char *entry, struct path_s *p)
{
	struct file_list_s *f;

	Get_Entry_From_Tree(entry, p, NULL, &f);

	if (f)
	{
		free(f->name);
		free(f->data);
		free(f->entry_name);
		if (f->previous)
		{
			if (f->next)
			{
				f->next->previous = f->previous;
				f->previous->next = f->next;
			}
			else
				f->previous->next = NULL;
		}
		else
		{
			if (f->next)
			{
				f->next->previous = NULL;
				f->dir->files = f->next;
			}
			else
			{
				f->dir->files = NULL;
			}
		}
		printf("??\n");
		free(f);
		printf("??\n");


		return 0;
	}


	return -1;




}





void Path_Get_File_Count_Size(struct path_s *pf, int *size, int *filecount)
{
	int _fc = 0;
	int _size = 0;
	struct path_list_s *pl;
	struct file_list_s *f;

	if (pf->files)
	{
		f = pf->files;
		while (f)
		{
			*size += f->length;
			*filecount += 1;
			f = f->next;
		}
	}

	pl = pf->entries;
	while (pl)
	{
		if (pl->path->entries || pl->path->files )
		{
			Path_Get_File_Count_Size(pl->path, size, filecount);
		}
		pl=pl->next;
	}
}

void Pak_Get_Size_File_Count(struct pak_s *pack, int *size, int *filecount)
{
	int _size = 0;

	Path_Get_File_Count_Size(pack->paths, &_size, filecount);
	*size = _size + 12 + 64 * (*filecount);


}

void Files_Create_Pack(struct path_s *pf, int *offset, char **buf, struct pak_file_s **fh)
{
	struct path_list_s *pl;
	struct file_list_s *f;

	if (pf->files)
	{
		f = pf->files;
		while (f)
		{
			(*fh)->len = f->length;
			strncpy((*fh)->name, f->entry_name, sizeof((*fh)->name));
			memcpy(*buf, f->data, f->length);
			(*fh)->pos = *offset;
			*offset += f->length;
			*buf += f->length;
			(*fh)++;
			f = f->next;
		}
	}

	pl = pf->entries;

	while (pl)
	{
		if (pl->path->entries || pl->path->files )
		{
			Files_Create_Pack(pl->path, offset, buf, fh);
		}
		pl=pl->next;
	}
}


char	*Create_Pak (struct pak_s *p, int *size)
{
	int 	_filecount = 0;
	int	_filesize = 0;
	int	_size = 0;
	char	*pbuf, *pfbuf;
	struct 	pak_header_s	*pheader;
	struct 	pak_file_s	*pfile;
	int	_fileoffset;

	Path_Get_File_Count_Size(p->paths, &_filesize, &_filecount);
	_filecount = 0;
	Pak_Get_Size_File_Count(p, &_size, &_filecount);

	*size = _size;

	printf("%i\n", _filecount);
	pbuf = malloc(_size);
	memset(pbuf,0,_size);

	pheader = pbuf;
	strncpy(pheader->id, "PACK", sizeof(pheader->id));
	pheader->dirofs = _filesize + 12;
	pheader->dirlen = _size - _filesize - 12;

	_fileoffset = 12;
	pfbuf = pbuf +12;
	pfile = pbuf + pheader->dirofs;

	Files_Create_Pack(p->paths, &_fileoffset, &pfbuf, &pfile);








	return pbuf;

}




void Paths_Print_Entries(struct path_s *p, int indent, int limit)
{
	struct path_list_s *pl;
	struct file_list_s *f;
	int i;

	if (p == NULL)
		return;
	pl = p->entries;

	printf("%s\n", p->name);
	if (p->files)
	{
		f = p->files;
		while (f)
		{
			printf("%s\n", f->name);
			f = f->next;
		}
	}

	while (pl)
	{
		if (pl->path->entries || pl->path->files )
		{
			Paths_Print_Entries(pl->path, indent + 2, limit);
		}
		pl=pl->next;
	}

}




void Paths_Get( struct pak_s *pack)
{
	char **pp, **ppb;
	char *fname;
	int i;
	struct pak_file_s *f;
	struct path_s		*s;
	struct file_list_s *ff;

	pack->paths = malloc(sizeof(struct path_s));
	memset(pack->paths, 0, (sizeof(struct path_s)));
	s = pack->paths;
	s->name = strdup("root");
	s->entries = NULL;
	s->files = NULL;

	f=pack->files;
	for (i=0 ; i<pack->num_files; i++)
	{
		ff = malloc(sizeof(struct file_list_s));
		memset(ff, 0, (sizeof(struct file_list_s)));
		ff-> length = f->len;
		ff->data = malloc(f->len);
		memcpy(ff->data, pack->buffer + f->pos, f->len);
		ff->name = get_filename(f->name);
		ff->next = NULL;
		if (Add_Entry_In_Tree(f->name, pack->paths, NULL, &ff) == -1)
			printf("error!\n");
		f++;
	}

}


void Paths_Clean (struct path_s *p)
{
	struct path_list_s *pl,*pln;
	struct file_list_s *fl,*fln;

	if (p->files)
	{
		fl = p->files;
		while (fl)
		{
			fln = fl->next;
			free(fl->name);
			free(fl->data);
			free(fl);
			fl = fln;
		}


	}

	if (p->entries)
	{
		pl = p->entries;
		while (pl)
		{
			Paths_Clean(pl->path);
			pln = pl->next;
			free(pl);
			pl = pln;
		}
	}
	free(p->name);
	free(p);
}

