#include <inc/string.h>

#include "fs.h"
//#define SET

unsigned char password[32]= "aes key for encryption";
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
	bool encrypted_status = false;
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

	encrypted_status = is_encrypted();
	if (!encrypted_status)
		encrypt_disk();
	else
		decrypt_disk();
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
	r = file_block_walk(f, filebno, &temp, true);
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

//	Encryption
bool
is_encrypted()
{
	int r;
	uint64_t *addr = NULL;
	r = sys_page_alloc(thisenv->env_id, addr, PTE_P|PTE_U|PTE_W);
        if (r<0)
                panic("is_encrypted : panic in sys_page_alloc\n");
	r = ide_read(KEYBLOCK * 8, addr, 8);
	if (r<0)
                panic("is_encrypted : panic in read\n");
	if (addr[0] == 0)
		return 0;
	else
		return 1;
}

void 
getkey()
{
	int r;
	unsigned char passwd[32];
	unsigned char ptext[16], ctext[16];
	void *plaintext = (void *)(UTEMP);
	void *ciphertext = (void *)(UTEMP + PGSIZE);
	
	r = sys_page_alloc(0, ciphertext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("getkey : panic in sys_page_alloc \n");
	r = sys_page_alloc(0, plaintext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("getkey : panic in sys_page_alloc \n");

	aes_set_key(&ctx, pwd, 128);

	r = ide_read(KEYBLOCK * 8, ciphertext, 8);
	if (r<0)
		panic("getkey : panic in read");

	for( r = 0; r < 4096; r = r + 16)
	{
		memcpy(ctext, (char *)(ciphertext + r), 16);
		aes_decrypt(&ctx, ctext, ptext);
		memcpy((char *)(plaintext + r), ptext, 16);
	}
	memcpy(passwd, plaintext + 4, sizeof(passwd));
	aes_set_key(&ctx, passwd, 128);
}

int
encrypt_block(uint32_t blockno, void *addr)
{
	int r, p;
	unsigned char ptext[16], ctext[16];
	void *temp = (void *)(UTEMP);

	getkey();
	r = sys_page_alloc(0, temp, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("encrypt_block : panic in sys_page_alloc\n");
		
	for( p=0; p<4096; p= p+16)
	{
		memcpy(ptext, (char *)(addr + p), 16);
		aes_encrypt(&ctx, ptext, ctext);
		memcpy((char *)(temp + p), ctext, 16);
	}
	ide_write(8 * blockno, temp, 8);
	return 0;
}


int
decrypt_block(uint32_t blockno, void *addr)
{
	int r, p;
	unsigned char ptext[16], ctext[16];
	void *plaintext = (void *)(UTEMP);
	void *ciphertext = (void *)(UTEMP + PGSIZE);

	getkey();	//Function to set the AES context variable
	r = sys_page_alloc(0, ciphertext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("decrypt_block : panic in sys page alloc\n");
	r = sys_page_alloc(0, plaintext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("decrypt_block : panic in sys page alloc\n");
	
	r = ide_read(blockno * 8, ciphertext, 8);

	if (r<0)
		panic(" decrypt_block : panic in read");
	
#ifdef SET 
	char temp[32];
	memcpy(temp, (char *)ciphertext, 32);
	//cprintf("blockno  %d\n", blockno);
	temp[32]='\0';
	if (1)
	{
		if (blockno == 1598)
		
			cprintf("cipher  %s\n", temp);
	}
#endif
	for( p = 0; p < 4096; p = p + 16)
	{
		memcpy(ctext, (char *)(ciphertext + p), 16);
		aes_decrypt(&ctx, ctext, ptext);
		memcpy((char *)(plaintext + p), ptext, 16);
	}
	memcpy(addr, plaintext, PGSIZE);
#ifdef SET 
	memcpy(temp, (char *)plaintext, 32);
	temp[32]='\0';
	if (blockno == 1598)
		cprintf("plaintext  %s\n", temp);
#endif
	return 0;
}

int
decrypt_disk()
{
	char *buf;
	char pass[32];
	int r, p;
	char special[4];
	char magicchar[4] ="2911";
	void *plaintext = (void *)(UTEMP);
	void *ciphertext = (void *)(UTEMP + PGSIZE);
	unsigned char ptext[16], ctext[16];

	cprintf("Disk is encrypted. You have to decrypt it before continuing\n");
	
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);
		
	r = sys_page_alloc(0, ciphertext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("panic in alloc block encryption");
	r = sys_page_alloc(0, plaintext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("panic in alloc block encryption");
		
	while(true)
	{		
		buf = readline("Enter password : ");
		
		strncpy(pass, buf, strlen(buf));
		memset(buf, 0, strlen(buf));
		memset(pwd, 0, strlen((char *)(pwd)));

		if (strlen(pass) == 0 )
		{
			cprintf("Nothing entered.\n");
			memset(pass, 0, strlen(pass));
			continue;
		}
		memcpy(pwd, pass, strlen(pass));
		memset(pass, 0, strlen(pass));

		aes_set_key(&ctx, pwd, 128);
		r = ide_read(KEYBLOCK * 8, ciphertext, 8);
		if (r<0)
			panic("decrypt_disk : panic in read\n");

		for( p = 0; p < 4096; p = p + 16)
		{
			memcpy(ctext, (char *)(ciphertext + p), 16);
			aes_decrypt(&ctx, ctext, ptext);
			memcpy((char *)(plaintext + p), ptext, 16);
		}
		memcpy(special, plaintext, sizeof(special));
		special[4]='\0';
		magicchar[4]='\0';
		if (strcmp(special, magicchar) == 0)
			return 1;
		else
		{
			memset(pwd, '\0', sizeof(char) * 32);
			cprintf("Wrong password. Please enter again\n");
		}
	}
	return 0;
}

int
encrypt_disk()
{
	char pass[32], repass[32];
	char *buf;
	int r, i, p;
	unsigned char key[32] = "aes key for encryption";
	unsigned char ptext[16], ctext[16];
	char magicchar[4] = "2911";
	void *plaintext = (void *)(UTEMP);
	void *ciphertext = (void *)(UTEMP + PGSIZE);
	void *temp = (void *)(UTEMP  + 2 * PGSIZE);
	
	cprintf("Disk is not encrypted. Please first encrypt disk\n");
	
	close(0);
	if ((r = opencons()) < 0)
		panic("opencons: %e", r);
	if (r != 0)
		panic("first opencons used fd %d", r);
	if ((r = dup(0, 1)) < 0)
		panic("dup: %e", r);

	while(true)
	{		
		buf = readline("Enter password to encrypt disk : ");
		
		strncpy(pass, buf, strlen(buf));
		memset(buf, 0, strlen(buf));
		if (strlen(pass) == 0 )
		{
			cprintf("Nothing entered.\n");
			memset(pass, 0, strlen(pass));
			continue;
		}
		
		buf = readline("Re enter password to confirm : ");
		strncpy(repass, buf, strlen(buf));
		memset(buf, 0, strlen(buf));
		if (strlen(repass) == 0)
		{
			cprintf("Nothing entered.\n");
			memset(repass, 0, strlen(repass));
			continue;
		}
		
		if (strcmp(pass, repass) == 0)
		{
			memcpy(pwd, pass, strlen(pass));
			break;
		}
		else
		{
			memset(pass, 0, strlen(pass));
			memset(repass, 0, strlen(repass));
			cprintf("Both passwords are not same \n");
		}
	}
	
	r = sys_page_alloc(0, ciphertext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("encrypt_disk : panic in sys_page_alloc");
	r = sys_page_alloc(0, plaintext, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("encrypt_disk : panic in sys_page_alloc");
	r = sys_page_alloc(0, temp, PTE_P|PTE_U|PTE_W);
	if (r<0)
		panic("encrypt_disk : panic in sys_page_alloc");
	
	r = alloc_block_encryption(KEYBLOCK);
	if (r<0)
		panic("encrypt_disk : KEYBLOCK allocation failed\n");

	cprintf("Encryption started. Please wait for some time\n");	
	memcpy(temp, &magicchar, sizeof(magicchar));
	memcpy(temp + sizeof(magicchar), key, sizeof(key));
	ide_write(KEYBLOCK * 8, temp, 8);
	aes_set_key(&ctx, pwd, 128);
	
	r = ide_read(KEYBLOCK * 8, plaintext, 8);
	if (r<0)
		panic("encrypt_disk : panic in read\n");
	for( p=0; p<4096; p= p+16)
	{
		memcpy(ptext, (char *)(plaintext + p), 16);
		aes_encrypt(&ctx, ptext, ctext);
		memcpy((char *)(ciphertext + p), ctext, 16);
	}
	ide_write(KEYBLOCK * 8, ciphertext, 8);
	
	aes_set_key(&ctx, key, 128);
	for (i = 3; i < super->s_nblocks; i++)
	{
		if (i == KEYBLOCK)
			continue;
		if(!block_is_free(i) )
		{
			memset(plaintext, 0, PGSIZE);
			memset(ciphertext, 0, PGSIZE);
			memset(ptext, 0, sizeof(ptext));
			memset(ctext, 0, sizeof(ctext));
			r = ide_read(i * 8, plaintext, 8);
			if (r<0)
				panic("encrypt_disk : panic in read\n");
			for( p = 0; p < 4096; p = p + 16)
			{
				memcpy(ptext, (char *)(plaintext + p), 16);
				aes_encrypt(&ctx, ptext, ctext);
				memcpy((char *)(ciphertext + p), ctext, 16);
			}
			ide_write(i * 8, ciphertext, 8);
		}
	}
	cprintf("Encryption is done\n"); 
	return 1;
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

int alloc_block_encryption(uint32_t blockno)
{
        if(block_is_free(blockno))
        {
        	bitmap[blockno/32] = bitmap[blockno/32] ^ (1 << (blockno%32));
            flush_block((void *)bitmap);
           	return 0;
        }
        return -E_NO_DISK;
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
