#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include "assign4.h"
// Some hard-coded inode numbers:
#define	ROOT_DIR	1
#define	ASSIGN_DIR	2
#define	USERNAME_FILE	3
#define FEATURE_FILE 4
#define	FILE_COUNT	1000//Maximum number of files

//sudo diskutil unmount force tmp

//Here
typedef struct file_node{
	bool is_dir;
	char name[50];
	// int child_count;
	// fuse_ino_t child_inode[50];//Maximum number of children - 50
	fuse_ino_t parent_inode;
}file_node;

// Hard-coded stat(2) information for each directory and file:
struct stat *file_stats;
struct file_node *helper_array;//Array of file node
int current_file_count = 4;

static const int AllRead = S_IRUSR | S_IRGRP | S_IROTH;
static const int AllExec = S_IXUSR | S_IXGRP | S_IXOTH;
static const int AllPermission = S_IRWXU | S_IRWXG | S_IRWXO;
// Hard-coded content of the "assignment/username" file:
static const char UsernameContent[] = "lx2786\n";
static const char FeatureContent[] = "Implemented following optional features:\n"
									  "-mkdir and rmdir\n"
									  "-ls\n"
									  "-create file and unlink file\n"
									  "-Permission manipulation\n";

static void
assign4_init(void *userdata, struct fuse_conn_info *conn)
{
	struct backing_file *backing = userdata;
	fprintf(stderr, "*** %s '%s'\n", __func__, backing->bf_path);

	file_stats = calloc(FILE_COUNT + 1, sizeof(*file_stats));
	helper_array = calloc(FILE_COUNT + 1, sizeof(struct file_node));

	file_stats[ROOT_DIR].st_ino = ROOT_DIR;
	file_stats[ROOT_DIR].st_mode = S_IFDIR | AllPermission | AllPermission;
	file_stats[ROOT_DIR].st_nlink = 1;
	helper_array[ROOT_DIR].is_dir = 1;//is a dir
	helper_array[ROOT_DIR].parent_inode = -1;//no parent 
	strcpy(helper_array[ROOT_DIR].name, "root");

	file_stats[ASSIGN_DIR].st_ino = ASSIGN_DIR;
	file_stats[ASSIGN_DIR].st_mode = S_IFDIR | AllRead | AllExec;
	file_stats[ASSIGN_DIR].st_nlink = 1;
	strcpy(helper_array[ASSIGN_DIR].name, "assignment");
	helper_array[ASSIGN_DIR].parent_inode = 1;
	//No need to add other helper to assignment dir since no file or dir are allowed to be created inside

	file_stats[USERNAME_FILE].st_ino = USERNAME_FILE;
	file_stats[USERNAME_FILE].st_mode = S_IFREG | AllRead;
	file_stats[USERNAME_FILE].st_size = sizeof(UsernameContent);
	file_stats[USERNAME_FILE].st_nlink = 1;
	strcpy(helper_array[USERNAME_FILE].name, "username");
	helper_array[USERNAME_FILE].parent_inode = 2;

	file_stats[FEATURE_FILE].st_ino = FEATURE_FILE;
	file_stats[FEATURE_FILE].st_mode = S_IFREG | AllRead;
	file_stats[FEATURE_FILE].st_size = sizeof(FeatureContent);
	file_stats[FEATURE_FILE].st_nlink = 1;
	strcpy(helper_array[FEATURE_FILE].name, "feature");
	helper_array[FEATURE_FILE].parent_inode = 2;
}

static void
assign4_destroy(void *userdata)
{
	struct backing_file *backing = userdata;
	fprintf(stderr, "*** %s %d\n", __func__, backing->bf_fd);

	free(file_stats);
	file_stats = NULL;
}

/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L256 */
static void
assign4_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info *fip)
{

	if (ino > current_file_count) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	int result = fuse_reply_attr(req, file_stats + ino, 1);
	if (result != 0) {
		fprintf(stderr, "Failed to send attr reply\n");
	}
}

/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L205 */
static void
assign4_lookup(fuse_req_t req, fuse_ino_t parent, const char *name)
{

	struct fuse_entry_param dirent;//ino+attr
	//
	printf("Here is parent %zu %s\n", parent, name);
	int ino = -1;
	struct stat file_stat;
	for(int i = 1; i<current_file_count+1;i++){
		printf("Here is parent %zu\n", helper_array[i].parent_inode);
		printf("inside %s\n", helper_array[i].name);
		if(parent==helper_array[i].parent_inode && strcmp(name, helper_array[i].name) == 0){
			printf("I got it %s\n");
			ino = i;
			file_stat= file_stats[i];
		}
	}
	if(ino>0){
		dirent.ino = ino;
		dirent.attr = file_stat;
	}
	else
	{
		// Let all other names fail
		fuse_reply_err(req, ENOENT);
		return;
	}

	// I'm not re-using inodes, so I don't need to worry about real
	// generation numbers... always use the same one.
	dirent.generation = 1;

	// Assume that these values are always valid for 1s:
	dirent.attr_timeout = 1;
	dirent.entry_timeout = 1;

	int result = fuse_reply_entry(req, &dirent);
	if (result != 0) {
		fprintf(stderr, "Failed to send dirent reply\n");
	}
}

/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L332 */
static void
assign4_mkdir(fuse_req_t req, fuse_ino_t parent, const char *name, mode_t mode)
{	
	if(parent != 2){
		struct fuse_entry_param dirent;//ino+attr
		current_file_count++;
		file_stats[current_file_count].st_ino = current_file_count;
		file_stats[current_file_count].st_mode = S_IFDIR | AllPermission | AllPermission;
		file_stats[current_file_count].st_nlink = 1;
		helper_array[current_file_count].is_dir = 1;//is a dir
		helper_array[current_file_count].parent_inode = parent;//no parent 
		// 
	
		strcpy(helper_array[current_file_count].name, name);
		// I'm not re-using inodes, so I don't need to worry about real
		// generation numbers... always use the same one.
		dirent.generation = 1;
		// Assume that these values are always valid for 1s:
		dirent.attr_timeout = 1;
		dirent.entry_timeout = 1;
		dirent.ino = current_file_count;
		dirent.attr = file_stats[current_file_count];
		int result = fuse_reply_entry(req,&dirent);
		if (result!=0){
			fprintf(stderr, "Failed to send dirent reply\n");
		}
	}
	else{
		fuse_reply_err(req,EPERM);
	}
}

/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L367 */
static void
assign4_rmdir(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	for(int i = 1; i<current_file_count+1;i++){
		printf("The value of i %d:\n", i);
		if(helper_array[i].parent_inode == parent && strcmp(name, helper_array[i].name) == 0){
			printf("I found the dir i want to delete");
			file_stats[i].st_ino = NULL;
			strcpy(helper_array[i].name,"");
			helper_array[i].parent_inode = -1;
			fuse_reply_err(req, 0);
		}
	}

}

/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L317 */
static void
assign4_mknod(fuse_req_t req, fuse_ino_t parent, const char *name,
           mode_t mode, dev_t rdev)
{	if(parent != 2){
		struct fuse_entry_param dirent;//ino+attr
		current_file_count++;
		file_stats[current_file_count].st_ino = current_file_count;
		file_stats[current_file_count].st_mode = mode;
		file_stats[current_file_count].st_nlink = 1;
		helper_array[current_file_count].is_dir = 1;//is a dir
		helper_array[current_file_count].parent_inode = parent;//no parent 
		// helper_array[current_file_count].child_count = 0;
		strcpy(helper_array[current_file_count].name, name);
		// I'm not re-using inodes, so I don't need to worry about real
		// generation numbers... always use the same one.
		dirent.generation = 1;
		// Assume that these values are always valid for 1s:
		dirent.attr_timeout = 1;
		dirent.entry_timeout = 1;
		dirent.ino = current_file_count;
		dirent.attr = file_stats[current_file_count];
		int result = fuse_reply_entry(req,&dirent);
		if (result!=0){
			fprintf(stderr, "Failed to send dirent reply\n");
		}
	}
	else{
		fuse_reply_err(req,EPERM);
	}
}


/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L622 */
static void
assign4_readdir(fuse_req_t req, fuse_ino_t ino, size_t size,
	     off_t off, struct fuse_file_info *fi)
{
	// In our trivial example, all directories happen to have three
	// entries: ".", ".." and either "assignment" or "username".
	if (off > 2) {
		fuse_reply_buf(req, NULL, 0);
		return;
	}

	struct stat *self = file_stats + ino;
	if (!S_ISDIR(self->st_mode)) {
		fuse_reply_err(req, ENOTDIR);
		return;
	}

	// In this trivial filesystem, the parent directory is always the root
	struct stat *parent;
	parent  = file_stats + ROOT_DIR;
	char buffer[size];
	off_t bytes = 0;
	int next = 0;

	if (off < 1)
	{
		bytes += fuse_add_direntry(req, buffer + bytes,
		                           sizeof(buffer) - bytes,
		                           ".", self, ++next);
	}

	if (off < 2)
	{
		bytes += fuse_add_direntry(req, buffer + bytes,
		                           sizeof(buffer) - bytes,
		                           "..", parent, ++next);
	}

	for (int j = 1; j<current_file_count+1; j++){
		if(helper_array[j].parent_inode == ino){
			bytes += fuse_add_direntry(req, buffer + bytes,
		                           sizeof(buffer) - bytes,
		                           helper_array[j].name,
		                           file_stats + file_stats[j].st_ino,
		                           ++next);
		}
	}
	int result = fuse_reply_buf(req, buffer, bytes);
	if (result != 0) {
		fprintf(stderr, "Failed to send readdir reply\n");
	}
}

/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L472 */
static void
assign4_read(fuse_req_t req, fuse_ino_t ino, size_t size,
	  off_t off, struct fuse_file_info *fi)
{
	const char *response_data = NULL;
	size_t response_len;
	int err = 0;

	switch (ino)
	{
	case ROOT_DIR:
	case ASSIGN_DIR:
		err = EISDIR;
		break;

	case USERNAME_FILE:
		if (off >= sizeof(UsernameContent)) {
			response_len = 0;
			break;
		}

		response_data = UsernameContent + off;
		response_len = sizeof(UsernameContent) - off;
		if (response_len > size) {
			response_len = size;
		}
		break;
	
	case FEATURE_FILE:
		if (off >= sizeof(FeatureContent)) {
			response_len = 0;
			break;
		}

		response_data = FeatureContent + off;
		response_len = sizeof(FeatureContent) - off;
		if (response_len > size) {
			response_len = size;
		}
		break;

	default:
		err = EBADF;
	}

	if (err != 0) {
		fuse_reply_err(req, err);
		return;
	}

	//assert(response_data != NULL);
	int result = fuse_reply_buf(req, response_data, response_len);
	if (result != 0) {
		fprintf(stderr, "Failed to send read reply\n");
	}
}



/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L286 */
static void
assign4_setattr(fuse_req_t req, fuse_ino_t ino, struct stat *attr,
	     int to_set, struct fuse_file_info *fi)
{	
	if (ino > current_file_count) {
		fuse_reply_err(req, ENOENT);
		return;
	}

	fprintf(stderr, "%s ino=%zu to_set=%d\n", __func__, ino, to_set);
	file_stats[ino].st_mode = attr->st_mode;
    int result = fuse_reply_attr(req, file_stats + ino, 1);
    if (result != 0) {
        fprintf(stderr, "Failed to send attr reply\n");
    }
}


/* https://github.com/libfuse/libfuse/blob/fuse_2_9_bugfix/include/fuse_lowlevel.h#L350 */
static void
assign4_unlink(fuse_req_t req, fuse_ino_t parent, const char *name)
{
	fprintf(stderr, "%s parent=%zu name='%s'\n", __func__, parent, name);
	for(int i = 1; i<current_file_count+1;i++){
		printf("The value of i %d:\n", i);
		if(helper_array[i].parent_inode == parent && strcmp(name, helper_array[i].name) == 0){
			printf("I found the dir i want to delete");
			file_stats[i].st_ino = NULL;
			strcpy(helper_array[i].name,"");
			helper_array[i].parent_inode = -1;
			fuse_reply_err(req, 0);
		}
	}
}



static struct fuse_lowlevel_ops assign4_ops = {
	.init           = assign4_init,
	.destroy        = assign4_destroy,
	.getattr        = assign4_getattr,
	.lookup         = assign4_lookup,
	.mkdir          = assign4_mkdir,
	.rmdir          = assign4_rmdir,
	.read           = assign4_read,
	.readdir        = assign4_readdir,
	.mknod          = assign4_mknod,
	.setattr        = assign4_setattr,
	.unlink         = assign4_unlink,
};


#if !defined(USE_EXAMPLE) && !defined(USE_SOLUTION)
struct fuse_lowlevel_ops*
assign4_fuse_ops()
{
	return &assign4_ops;
}
#endif