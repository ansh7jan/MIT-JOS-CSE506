#include <inc/string.h>

#include "fs.h"

// --------------------------------------------------------------
// Super block
// --------------------------------------------------------------

// Validate the file system super-block.
void
check_super(void)
{
	if (super->s_magic != FS_MAGIC)
		panic("bad file system magic number");

	if (super->s_nblocks > DISKSIZE/BLKSIZE)
		panic("file system is too large");

	cprintf("superblock is good\n");
}


// --------------------------------------------------------------
// File system structures
// --------------------------------------------------------------

// Initialize the file system
void
fs_init(void)
{
	static_assert(sizeof(struct File) == 256);

	// Find a JOS disk.  Use the second IDE disk (number 1) if available.
	if (ide_probe_disk1())
		ide_set_disk(1);
	else
		ide_set_disk(0);
	bc_init();

	// Set "super" to point to the super block.
	super = diskaddr(1);
        bitmap = diskaddr(2);
	check_super();
}

// Find the disk block number slot for the 'filebno'th block in file 'f'.
// Set '*ppdiskbno' to point to that slot.
// The slot will be one of the f->f_direct[] entries,
// or an entry in the indirect block.
// When 'alloc' is set, this function will allocate an indirect block
// if necessary.
//
//  Note, for the read-only file system (lab 5 without the challenge), 
//        alloc will always be false.
//
// Returns:
//	0 on success (but note that *ppdiskbno might equal 0).
//	-E_NOT_FOUND if the function needed to allocate an indirect block, but
//		alloc was 0.
//	-E_NO_DISK if there's no space on the disk for an indirect block.
//	-E_INVAL if filebno is out of range (it's >= NDIRECT + NINDIRECT).
//
// Analogy: This is like pgdir_walk for files.
// Hint: Don't forget to clear any block you allocate.
static int
file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
{
        // LAB 5: Your code here.
	int r;
	uint32_t * temp;

	if (filebno >= NDIRECT + NINDIRECT)
		return -E_INVAL;

	if (filebno < NDIRECT)
	{
		*ppdiskbno =(uint32_t *) &(f->f_direct[filebno]);
		return 0;
	}
	if (!f->f_indirect)
	{
		if (alloc == false)
			return -E_NOT_FOUND;
		else
		{	
                        r = alloc_block();
			if (r<0)
				return -E_NO_DISK;
			memset(diskaddr(r), 0, BLKSIZE);	 
			f->f_indirect = r;
			temp = (uint32_t *)diskaddr(f->f_indirect);
                	*ppdiskbno = (temp + filebno - NDIRECT);
                	return 0;
		}
		
	}
	else
	{
		temp = (uint32_t *)diskaddr(f->f_indirect);
		*ppdiskbno = (temp + filebno - NDIRECT);
		return 0;
	}
	
	return 0;	
      //  panic("file_block_walk not implemented");
}

// Set *blk to the address in memory where the filebno'th
// block of file 'f' would be mapped.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_NO_DISK if a block needed to be allocated but the disk is full.
//	-E_INVAL if filebno is out of range.
//
int
file_get_block(struct File *f, uint32_t filebno, char **blk)
{
	// LAB 5: Your code here.
	int r;
	uint32_t *temp;
	if (filebno >= NDIRECT + NINDIRECT)
		return -E_INVAL;
	r = file_block_walk(f, filebno, &temp, false);
	if (r<0)
		return r;
	if (*temp == 0)
	{
                r = alloc_block();
                if (r<0)
			return -E_NO_DISK; 	
		 *temp = r;
	}
	*blk = (char *)diskaddr(*temp);
	return 0;

	//panic("file_block_walk not implemented");
}

// Try to find a file named "name" in dir.  If so, set *file to it.
//
// Returns 0 and sets *file on success, < 0 on error.  Errors are:
//	-E_NOT_FOUND if the file is not found
static int
dir_lookup(struct File *dir, const char *name, struct File **file)
{
	int r;
	uint32_t i, j, nblock;
	char *blk;
	struct File *f;
	// Search dir for name.
	// We maintain the invariant that the size of a directory-file
	// is always a multiple of the file system's block size.
	assert((dir->f_size % BLKSIZE) == 0);
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
			if (strcmp(f[j].f_name, name) == 0) {
				*file = &f[j];
				return 0;
			}
	}
	return -E_NOT_FOUND;
}


// Skip over slashes.
static const char*
skip_slash(const char *p)
{
	while (*p == '/')
		p++;
	return p;
}

// Evaluate a path name, starting at the root.
// On success, set *pf to the file we found
// and set *pdir to the directory the file is in.
// If we cannot find the file but find the directory
// it should be in, set *pdir and copy the final path
// element into lastelem.
static int
walk_path(const char *path, struct File **pdir, struct File **pf, char *lastelem)
{
	const char *p;
	char name[MAXNAMELEN];
	struct File *dir, *f;
	int r;

	// if (*path != '/')
	//	return -E_BAD_PATH;
	path = skip_slash(path);
	f = &super->s_root;
	dir = 0;
	name[0] = 0;

	if (pdir)
		*pdir = 0;
	*pf = 0;
	while (*path != '\0') {
		dir = f;
		p = path;
		while (*path != '/' && *path != '\0')
			path++;
		if (path - p >= MAXNAMELEN)
			return -E_BAD_PATH;
		memmove(name, p, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (dir->f_type != FTYPE_DIR)
			return -E_NOT_FOUND;

		if ((r = dir_lookup(dir, name, &f)) < 0) {
			if (r == -E_NOT_FOUND && *path == '\0') {
				if (pdir)
					*pdir = dir;
				if (lastelem)
					strcpy(lastelem, name);
				*pf = 0;
			}
			return r;
		}
	}

	if (pdir)
		*pdir = dir;
	*pf = f;
	return 0;
}

// --------------------------------------------------------------
// File operations
// --------------------------------------------------------------


// Open "path".  On success set *pf to point at the file and return 0.
// On error return < 0.
int
file_open(const char *path, struct File **pf)
{
	return walk_path(path, 0, pf, 0);
}

// Read count bytes from f into buf, starting from seek position
// offset.  This meant to mimic the standard pread function.
// Returns the number of bytes read, < 0 on error.
ssize_t
file_read(struct File *f, void *buf, size_t count, off_t offset)
{
	int r, bn;
	off_t pos;
	char *blk;

	if (offset >= f->f_size)
		return 0;

	count = MIN(count, f->f_size - offset);

	for (pos = offset; pos < offset + count; ) {
		if ((r = file_get_block(f, pos / BLKSIZE, &blk)) < 0)
			return r;
		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
		memmove(buf, blk + pos % BLKSIZE, bn);
		pos += bn;
		buf += bn;
	}

	return count;
}

// Same almost as dir_lookup
// Need to be done while creating new file 
static int
dir_alloc_file(struct File *dir, struct File **file)
{
	int r;
	uint32_t nblock, i, j;
	char *blk;
	struct File *f;
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
			if (f[j].f_name[0] == '\0') {
				*file = &f[j];
				return 0;
			}
	}
	dir->f_size = dir->f_size + BLKSIZE;
        r = file_get_block(dir, i, &blk);
	if (r<0)
		return r;
	f = (struct File*) blk;
	*file = f;
	return 0;
}

// Similar like read
//Point 5 Challenge
bool
block_is_free(uint32_t blockno)
{
        if (blockno == 0 || (super && blockno >= super->s_nblocks))
                return 0;
	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;
	return 0;
}

int alloc_block(void)
{
	int i;
	for(i = 1; i < super->s_nblocks; i++)
	{
		if(block_is_free(i))
		{
			bitmap[i/32] = bitmap[i/32] ^ (1 << (i%32));
			flush_block((void *)bitmap);
			return i;
		}
	}
	return -E_NO_DISK;
}

void
free_block(uint32_t blockno)
{
	if (blockno == 0 || blockno == 1)
		panic("cannot free zero and ist block block\n");
	bitmap[blockno/32] |= 1<<(blockno%32);
}

int
file_write(struct File *f, const void *buf, size_t count, off_t offset)
{
	int r, bn;
	off_t pos;
	char *blk;
	if (offset + count > f->f_size)
        {
		r = file_set_size(f, offset + count);
                if (r<0)
			return r;
          }
	for (pos = offset; pos < offset + count; ) {
                r = file_get_block(f, pos / BLKSIZE, &blk);
		if (r<0)
			return r;
		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
		memmove(blk + pos % BLKSIZE, buf, bn);
		pos += bn;
		buf += bn;
	}
	return count;
}

int
file_create(const char *path, struct File **pf)
{
	char file[100];
	int r;
	struct File *directory, *f;
        r = walk_path(path, &directory, &f, file);
	if (r == 0)
		return -E_FILE_EXISTS;
	if (directory == 0 || r != -E_NOT_FOUND)
		return r;
        r = dir_alloc_file(directory, &f);
	if (r<0)
		return r;
	memcpy(f->f_name, file, strlen(file));
	file_flush(directory);
	*pf = f;
	return 0;
}

int
file_set_size(struct File *f, off_t newsize)
{
        uint32_t *ptr;
	int i;
	if (f->f_size > newsize)
        {
                newsize /= BLKSIZE;
		if (newsize <= NDIRECT && newsize != 0)
                free_block(f->f_indirect);

	for (i = newsize; i < (f->f_size / BLKSIZE); i++)
	{
                 if (i == 0 || i == 1)
                         continue;
                  file_block_walk(f, i, &ptr, 0);
	          if (*ptr)
		        free_block(*ptr);
          }
        }
	f->f_size = newsize;
	flush_block(f);
	return 0;
}

int
file_remove(const char *path)
{
	uint32_t *ptr;
	int i;
        int newsize;
        int r;
	struct File *f;
        r = walk_path(path, 0, &f, 0);
	if (r<0)
		return r;
	for (i = 2; i < (f->f_size / BLKSIZE); i++)
	{
                  file_block_walk(f, i, &ptr, 0);
	          if (*ptr)
		        free_block(*ptr);
          }
        f->f_size = 0;
        memset(f->f_name, 0, sizeof(f->f_name));
	flush_block(f);
	return 0;
}

void
file_flush(struct File *f)
{
	int i;
	uint32_t *ptr;
	if (f->f_indirect)
		flush_block(diskaddr(f->f_indirect));
	for (i = 0; i<(f->f_size + BLKSIZE - 1) / BLKSIZE; i++)
         {
                file_block_walk(f, i, &ptr, 0);
                if (*ptr)
		        flush_block(diskaddr(*ptr));
	}
	flush_block(f);  
}