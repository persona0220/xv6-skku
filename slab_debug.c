#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"
#include "slab.h"

struct {
	struct spinlock lock;
	struct slab slab[NSLAB];
} stable;

void slabinit(){
	acquire(&stable.lock);
	cprintf("init ...");
	/* fill in the blank */
	int i, j;
	int s = 8;
	for(i=0; i<NSLAB; i++){
		stable.slab[i].size = s;
		stable.slab[i].num_pages = 1;
		stable.slab[i].num_free_objects = 4096/stable.slab[i].size;
		stable.slab[i].num_used_objects = 0;
		stable.slab[i].num_objects_per_page = 4096/stable.slab[i].size;
		stable.slab[i].bitmap = kalloc(); 

		if(stable.slab[i].size == 1024){
			for(j=0; j<4096; j++){
				stable.slab[i].bitmap[j]= -16;
			}
		}
		else if(stable.slab[i].size == 2048){
			for(j=0; j<4096; j++){
				stable.slab[i].bitmap[j]= -4;
			}
		}
		else{
			for(j=0; j<4096; j++){
				stable.slab[i].bitmap[j]= 0;
			}
		}

		stable.slab[i].page[0] = kalloc();
		
		s *= 2;
	}
	cprintf("init finished\n");
	release(&stable.lock);
}
/*
int find_empty_page(int slabno){
	
	//if a page is all empty
	int isempty = 1;
	int i, j;
	int char_per_page = stable.slab[slabno].num_objects_per_page/8;
	if(char_per_page == 0) char_per_page++;

	for(i=0; i<MAX_PAGES_PER_SLAB; i++){
		isempty=1;
		for(j = 0; j<char_per_page; j++){
			if((slabno < 7 && stable.slab[slabno].bitmap[i*char_per_page+j] != 0) || (slabno == 7 && stable.slab[slabno].bitmap[i] != -16) || (slabno == 8 && stable.slab[slabno].bitmap[i]!= -4)){
				isempty = 0;
				break;
			}
		}

		if(isempty == 1){
			cprintf("//%d th page is empty.//", i);
			return i;
		}
	}

	return -1;
}
*/

char *kmalloc(int size){
	/* fill in the blank */
	acquire(&stable.lock);
	int i = 0;
	int eight = 8;
	
	while(size > eight){
		eight *= 2;
		i++;
	}

	int char_per_page = stable.slab[i].num_objects_per_page/8;
	if(char_per_page == 0) char_per_page++;

	cprintf("kmalloc started...%d, %d, %d, [%d.%d.%d]",size, stable.slab[i].size, stable.slab[i].num_free_objects, stable.slab[i].bitmap[0], stable.slab[i].bitmap[1], stable.slab[i].bitmap[2]);	
	if(stable.slab[i].num_free_objects == 0){
		//need more page..
		cprintf("need more page...");

		//make new page, using kalloc.
		int e = stable.slab[i].num_pages;
		stable.slab[i].num_pages++;
		stable.slab[i].page[e] = kalloc();
		stable.slab[i].num_free_objects = stable.slab[i].num_objects_per_page;
		
		//put a new object in the newly allocated page's first slot.
		stable.slab[i].num_free_objects--;
		stable.slab[i].num_used_objects++;
		stable.slab[i].bitmap[char_per_page*e] += 1;
		
		release(&stable.lock);
		cprintf(".. %dpage, %dobj, (%x)\n",e, 0, stable.slab[i].page[e]);
		return stable.slab[i].page[e];
	}
	else{
		//find the free slot
		cprintf("find...");
		int k;
		int page = 0, obj = 0;
		for(k = 0; k<4096; k++){
			if(stable.slab[i].bitmap[k] != -1){
				//free object in this char!
				
				int bit = (int)(unsigned char)stable.slab[i].bitmap[k];
				
				cprintf("bit = %d/", bit);
				int o = 0;
				int o2 = 1;
				while(bit != 0){
					if(bit % 2 == 0){
						break;
					}
					else{
						o++;
						o2 *= 2;
						bit /= 2;
					}
				}
				//first free object is at 'o'. if o = 3, 1111 0111 or 0000 0111 should be result.
				//modify bitmap
				stable.slab[i].num_free_objects--;
				stable.slab[i].num_used_objects++;
				stable.slab[i].bitmap[k] += o2;
				if(k == 0){
					page = 0;
					obj = o;
				}
				else{
					page = k/char_per_page;
					obj = (k%char_per_page)*8 + o;
				}
				break;
			}
		}
		cprintf("existing page.. %dpage, %dobj, %x\n", page, obj, stable.slab[i].page[page]+obj*stable.slab[i].size);
		release(&stable.lock);
		return stable.slab[i].page[page] + obj*stable.slab[i].size; 
	}
}

void kmfree(char *addr){
	acquire(&stable.lock);
	cprintf("kmfree start...finding %x..", addr);
	
	int find = 0;
	int i,j;
	for(i = 0; i<NSLAB; i++){
		for(j = 0; j < MAX_PAGES_PER_SLAB; j++){
			if (stable.slab[i].page[j] <= addr && addr <= stable.slab[i].page[j] + (stable.slab[i].num_objects_per_page-1)*stable.slab[i].size){
			cprintf("//%x< %x < %x(page%d)//", stable.slab[i].page[j], addr, stable.slab[i].page[j] +(stable.slab[i].num_objects_per_page-1)*stable.slab[i].size, j);
				find = 1;
				break;
			}
		}	
		if(find == 1) break;
	}
	stable.slab[i].num_free_objects++;
	stable.slab[i].num_used_objects--;

	//uncheck bitmap
	int page = j;
	int char_per_page = stable.slab[i].num_objects_per_page/8;
	if(char_per_page == 0) char_per_page++;

	int obj = (int)(addr - stable.slab[i].page[j]);
	int k = page*char_per_page;
	int o = 0;
	if (obj != 0){
		obj /= stable.slab[i].size;
		k += obj/8;
		o = obj%8;
	}
	int o2 = 1;
	while(o--){
		o2*=2;
	}
	stable.slab[i].bitmap[k] -= o2;
	cprintf("size = %d, page = %d, obj = %d... \n", stable.slab[i].size, page, obj);
	release(&stable.lock);
}

void slabdump(){
	struct slab *s;

	cprintf("size\tnum_pages\tused_objects\tfree_objects\n");
	for(s = stable.slab; s < &stable.slab[NSLAB]; s++){
		cprintf("%d\t%d\t\t%d\t\t%d\n", s->size, s->num_pages, s->num_used_objects, s->num_free_objects);
	}
}
