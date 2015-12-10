

/**
 * Copyright (c) 2015, Harrison Bowden, Minneapolis, MN
 * 
 * Permission to use, copy, modify, and/or distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright notice 
 * and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH 
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY 
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, 
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 **/

#include "resource.h"
#include "concurrent.h"
#include "nextgen.h"
#include "platform.h"
#include "network.h"
#include "crypto.h"
#include "utils.h"
#include "memory.h"
#include "io.h"

#include <sys/param.h>
#include <sys/mount.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

static struct mem_pool_shared *desc_pool;

static struct mem_pool_shared *mount_pool;

static struct mem_pool_shared *dirpath_pool;

static struct mem_pool_shared *file_pool;

static struct mem_pool_shared *socket_pool;

int32_t get_desc(void)
{
    int32_t *fd = NULL;
	struct memory_block *m_blk = NULL;

    /* Grab a shared memory block from the decriptor pool. */
	m_blk = mem_get_shared_block(desc_pool);
	if(m_blk == NULL)
	{
		output(ERROR, "Can't get shared block\n");
		return (-1);
	}

    /* Get resource pointer. */
    struct resource_ctx *resource = (struct resource_ctx *)m_blk->ptr;

    /* Set fd to the descriptor we grabbed from the pool. */
	fd = (int32_t *)resource->ptr;

	return (*fd);
}

int32_t free_desc(int *fd)
{	

	return 0;
}

int32_t get_mount(char **path)
{
	return 0;
}

int32_t free_mount(char **path)
{

	return 0;
}

char *get_dirpath(void)
{
    char *path = NULL;
    struct memory_block *m_blk = NULL;

    /* Grab a shared memory block from the dirpath pool. */
    m_blk = mem_get_shared_block(dirpath_pool);
    if(m_blk == NULL)
    {
        output(ERROR, "Can't get shared block\n");
        return NULL;
    }

    /* Get resource pointer. */
    struct resource_ctx *resource = (struct resource_ctx *)m_blk->ptr;

    /* Set fd to the descriptor we grabbed from the pool. */
    path = (char *)resource->ptr;

    return (path);
}

int32_t free_dirpath(char **path)
{

	return 0;
}

char *get_filepath(void)
{
    char *path = NULL;
    struct memory_block *m_blk = NULL;

    /* Grab a shared memory block from the file pool. */
    m_blk = mem_get_shared_block(file_pool);
    if(m_blk == NULL)
    {
        output(ERROR, "Can't get shared block\n");
        return NULL;
    }

    /* Get resource pointer. */
    struct resource_ctx *resource = (struct resource_ctx *)m_blk->ptr;

    /* Set fd to the descriptor we grabbed from the pool. */
    path = (char *)resource->ptr;

	return (path);
}

int32_t free_filepath(char **path)
{

	return 0;
}

static int32_t create_fd_pool(char *path)
{
    int32_t rtrn = 0;
	struct memory_block *m_blk = NULL;

	desc_pool = mem_create_shared_pool(sizeof(int *), POOL_SIZE);
	if(desc_pool == NULL)
	{
		output(ERROR, "Can't allocate descriptor memory pool\n");
		return -1;
	}

    /* Create a bunch of file descriptors and stick them into the resource pool. */
    init_shared_pool(&desc_pool, m_blk)
    {
    	/* Temp variables that we define with auto_clean so that we 
        don't have to worry about calling free. */
        char *file_name auto_clean = NULL;
        char *junk auto_clean = NULL;
        char *file_path auto_clean = NULL;

        /* Don't free that will be taken cared off later. */
        struct resource_ctx *resource = NULL;

        resource = mem_alloc_shared(sizeof(struct resource_ctx));
        if(resource == NULL)
        {
        	output(ERROR, "Can't alloc resource object\n");
        	return (-1);
        }

        resource->ptr = mem_alloc_shared(sizeof(int32_t *));
        if(resource->ptr == NULL)
        {
            output(ERROR, "Can't alloc resource pointer\n");
            return (-1);
        }

        /* Use generate_name to create a random file name with the text extension.  */
        rtrn = generate_name((char **)&file_name, ".txt", FILE_NAME);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't generate file path\n");
    	    return (-1);
        }

        /* Join the file name with the home path. */
        rtrn = asprintf(&file_path, "%s/%s", path, file_name);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't join paths: %s\n", strerror(errno));
    	    return (-1);
        }

        /* Allocate a buffer to put junk in. */
        junk = mem_alloc(4096);
        if(junk == NULL)
        {
            output(ERROR, "Can't alloc junk buf: %s\n", strerror(errno));
    	    return (-1);
        }

        /* Put some junk in a buffer. */
        rtrn = rand_bytes(&junk, 4095);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't alloc junk buf: %s\n", strerror(errno));
    	    return (-1);
        }

        /* Write that junk to the file so that the file is not just blank. */
        rtrn = map_file_out(file_path, junk, 4095);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't write junk to disk\n");
    	    return -1;
        }

    	int32_t fd = open(file_path, O_RDWR, 0777);
    	if(fd < 0)
    	{
            output(ERROR, "Can't open newly created file: %s\n", strerror(errno));
            return -1;

    	}

        memmove(resource->ptr, &fd, sizeof(int32_t));

        resource->m_blk = m_blk;

        m_blk->ptr = resource;
    }

	return (0);
}

static int32_t create_file_pool(char *path)
{
    int32_t rtrn = 0;
	struct memory_block *m_blk = NULL;

	file_pool = mem_create_shared_pool(sizeof(char *), POOL_SIZE);
	if(file_pool == NULL)
	{
		output(ERROR, "Can't allocate file path memory pool\n");
		return (-1);
	}

    init_shared_pool(&file_pool, m_blk)
    {
    	/* Temp variables that we define with auto_clean so that we 
        don't have to worry about calling free. */
        char *file_name auto_clean = NULL;
        char *junk auto_clean = NULL;
      
        /* Don't free, that will be taken cared off later. */
        char *file_path = NULL;
        struct resource_ctx *resource = NULL;

        resource = mem_alloc_shared(sizeof(struct resource_ctx));
        if(resource == NULL)
        {
            output(ERROR, "Can't alloc resource object\n");
            return (-1);
        }

        resource->ptr = mem_alloc_shared(PATH_MAX + 1);
        if(resource->ptr == NULL)
        {
            output(ERROR, "Can't alloc resource pointer\n");
            return (-1);
        }

        /* Use generate_name to create a random file name with the text extension.  */
        rtrn = generate_name((char **)&file_name, ".txt", FILE_NAME);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't generate file path\n");
    	    return (-1);
        }

        /* Join the file name with the home path. */
        rtrn = asprintf(&file_path, "%s/%s", path, file_name);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't join paths: %s\n", strerror(errno));
    	    return -1;
        }

        /* Allocate a buffer to put junk in. */
        junk = mem_alloc(4096);
        if(junk == NULL)
        {
            output(ERROR, "Can't alloc junk buf: %s\n", strerror(errno));
    	    return -1;
        }

        /* Put some junk in a buffer. */
        rtrn = rand_bytes(&junk, 4095);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't alloc junk buf: %s\n", strerror(errno));
    	    return -1;
        }

        /* Write that junk to the file so that the file is not just blank. */
        rtrn = map_file_out(file_path, junk, 4095);
        if(rtrn < 0)
        {
    	    output(ERROR, "Can't write junk to disk\n");
    	    return -1;
        }

        memmove(resource->ptr, file_path, strlen(file_path));

        resource->m_blk = m_blk;

        m_blk->ptr = resource;
    }
	return (0);
}

/*static int create_mount_pool(void)
{
	map->mount_pool = mem_create_shared_pool(sizeof(char *), 1024);
	if(map->mount_pool == NULL)
	{
		output(ERROR, "Can't allocate mount path memory pool\n");
		return -1;
	}

    int rtrn;
	struct memory_block *block = NULL;

    CK_SLIST_FOREACH(block, &map->mount_pool->free_list, list_entry)
    {
        char *mount_name auto_clean = NULL;
        char *fs_name auto_clean = NULL;

        char *mount_path = NULL;

        rtrn = generate_name((char **)&mount_name, NULL, DIR_NAME);
        if(rtrn < 0)
        {
            output(ERROR, "Can't generate dir name\n");
            return -1;
        }

        rtrn = generate_name((char **)&fs_name, NULL, DIR_NAME);
        if(rtrn < 0)
        {
            output(ERROR, "Can't generate dir name\n");
            return -1;
        }

        rtrn = asprintf(&mount_path, "/tmp/%s", mount_name);
        if(rtrn < 0)
        {
            output(ERROR, "Can't join paths: %s\n", strerror(errno));
            return -1;
        }
        output(STD, "path: %s\n", mount_path);
        rtrn = mount(fs_name, mount_path, MNT_FORCE, NULL);
        if(rtrn < 0)
        {
           output(ERROR, "mount: %s\n", strerror(errno));
           return -1;
        }

        block->ptr = mount_path;
    }

	return 0;
}*/

static int32_t create_dirpath_pool(void)
{
	dirpath_pool = mem_create_shared_pool(sizeof(char *), POOL_SIZE);
	if(dirpath_pool == NULL)
	{
		output(ERROR, "Can't allocate dir path memory pool\n");
		return (-1);
	}

    int32_t rtrn = 0;
	struct memory_block *m_blk = NULL;

    SLIST_FOREACH(m_blk, &dirpath_pool->free_list, list_entry)
    {
        /* Temp variable that we define with auto_clean so that we 
        don't have to worry about calling free. */
        char *dir_name auto_clean = NULL;
        
        /* Don't free, that will be taken cared off later. */
        char *dir_path = NULL;
        struct resource_ctx *resource = NULL;

        resource = mem_alloc_shared(sizeof(struct resource_ctx));
        if(resource == NULL)
        {
            output(ERROR, "Can't alloc resource object\n");
            return (-1);
        }

        resource->ptr = mem_alloc_shared(PATH_MAX + 1);
        if(resource->ptr == NULL)
        {
            output(ERROR, "Can't alloc resource pointer\n");
            return (-1);
        }

        /* Use generate_name to create a random directory name.  */
        rtrn = generate_name((char **)&dir_name, NULL, DIR_NAME);
        if(rtrn < 0)
        {
           output(ERROR, "Can't generate file path\n");
           return -1;
        }

        /* Join the dir name with the temp path. */
        rtrn = asprintf(&dir_path, "/tmp/%s", dir_name);
        if(rtrn < 0)
        {
           output(ERROR, "Can't join paths: %s\n", strerror(errno));
           return -1;
        }

        memmove(resource->ptr, dir_path, strlen(dir_path));

        resource->m_blk = m_blk;

        m_blk->ptr = resource;
    }

	return 0;
}

static int32_t clean_file_pool(void)
{
    int32_t rtrn = 0;
    struct memory_block *m_blk = NULL;

    if(file_pool == NULL)
    {
        output(ERROR, "File pool already clean\n");
        return (-1);
    }

    assert(&file_pool->allocated_list != NULL);

    /* Loop and grab all the file paths in the allocated list. */
    SLIST_FOREACH(m_blk, &file_pool->allocated_list, list_entry)
    {
        struct resource_ctx *resource = (struct resource_ctx *)m_blk->ptr;

        rtrn = unlink((char *)resource->ptr);
        if(rtrn < 0)
        {
            output(ERROR, "Can't remove temp file\n");
            return (-1);
        }

        mem_free_shared(resource->ptr, PATH_MAX + 1);
        mem_free_shared(m_blk->ptr, sizeof(struct resource_ctx));
        mem_free_shared(m_blk, sizeof(struct memory_block));
    }

    /* Now check the free list for any resource blocks. */
    SLIST_FOREACH(m_blk, &file_pool->free_list, list_entry)
    {
        struct resource_ctx *resource = (struct resource_ctx *)m_blk->ptr;

        rtrn = unlink((char *)resource->ptr);
        if(rtrn < 0)
        {
            output(ERROR, "Can't remove temp file\n");
            return (-1);
        }

        mem_free_shared(resource->ptr, PATH_MAX + 1);
        mem_free_shared(m_blk->ptr, sizeof(struct resource_ctx));
        mem_free_shared(m_blk, sizeof(struct memory_block));
    }

    mem_clean_shared_pool(file_pool);

    return (0);
}

int32_t cleanup_resource_pool(void)
{
    int32_t rtrn = 0;

    rtrn = clean_file_pool();
    if(rtrn < 0)
    {
        output(ERROR, "Can't clean file pool\n");
        return (-1);
    }

    return (0);
}

int32_t setup_resource_module(char *path)
{
    output(STD, "Creating resource pools\n");

	int32_t rtrn = 0;

    /* Start socket server. We use this to connect to, to create loopback sockets. */
    rtrn = setup_network_module(SOCKET_SERVER);
    if(rtrn < 0)
    {
        output(ERROR, "Can't start socket server\n");
        return -1;
    }

/*	rtrn = create_mount_pool();
	if(rtrn < 0)
	{
		output(ERROR, "Can't create mount pool\n");
		return -1;
	} */

	rtrn = create_file_pool(path);
	if(rtrn < 0)
	{
		output(ERROR, "Can't create file pool\n");
		return -1;
	}

	rtrn = create_fd_pool(path);
	if(rtrn < 0)
	{
		output(ERROR, "Can't create fd pool\n");
		return -1;
	}

	rtrn = create_dirpath_pool();
	if(rtrn < 0)
	{
		output(ERROR, "Can't create fd pool\n");
		return -1;
	}

	return 0;
}

