#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

//Nb max char in a line
#define LINE_SIZE = 35

//Value is set on the first read
static int BUFFER_SIZE = 4096;
//To have the proc data length
static int len = 0;
static char * tmpBuf = NULL;
static int stop = 0;


//Version /proc
void getProcTreeForProc(struct task_struct *task, int cpt, char * buf)
{
  struct task_struct *child;
  struct list_head *list;
  //Check policy
  char policy[512];
  unsigned int policyInt = task->policy;
  int priority = 0;
  if (policyInt == SCHED_NORMAL)
  {
    sprintf(policy,"NORMAL");
    priority = task->prio;
  }
  else if (policyInt == SCHED_FIFO)
  {
    sprintf(policy,"FIFO");
    priority = task->rt_priority;
  }
  else if (policyInt == SCHED_BATCH)
  {
    sprintf(policy,"BATCH");
    priority = task->rt_priority;
  }
  else if (policyInt == SCHED_RR)
  {
    sprintf(policy,"RR");
    priority = task->prio;
  }
  else
  {
    sprintf(policy,"IDLE");
    priority = task->prio;
  }
  //Create the hierachy using _
  char indent[64] = "\0";
  int i = 0;
  while (i < cpt)
  {
    sprintf(indent + i, "_");
    ++i;
  }
  //if our buffer is too small, take more memory
  if (BUFFER_SIZE - len < 35)
  {
    krealloc(buf, BUFFER_SIZE * 2, GFP_USER);
    BUFFER_SIZE = BUFFER_SIZE * 2;
  }
  //Concat proc data to the buffer
  len += sprintf(buf + len,"%s%s (pid=%d) %s, priority=%d\n",indent, task->comm ,task->pid, policy, priority);
  list_for_each(list, &task->children)
  {
    child = list_entry(list, struct task_struct, sibling);
    //recursive call
    getProcTreeForProc(child, cpt + 1, buf);
  }
}


//Version 2
void getProcTree(struct task_struct *task, int cpt)
{
  struct task_struct *child;
  struct list_head *list;

  //Check policy
  char policy[512];
  unsigned int policyInt = task->policy;
  int priority = 0;
  if (policyInt == SCHED_NORMAL)
  {
    sprintf(policy,"NORMAL");
    priority = task->prio;
  }
  else if (policyInt == SCHED_FIFO)
  {
    sprintf(policy,"FIFO");
    priority = task->rt_priority;
  }
  else if (policyInt == SCHED_BATCH)
  {
    sprintf(policy,"BATCH");
    priority = task->rt_priority;
  }
  else if (policyInt == SCHED_RR)
  {
    sprintf(policy,"RR");
    priority = task->prio;
  }
  else
  {
    sprintf(policy,"IDLE");
    priority = task->prio;
  }
  //Create the hierachy using
  char indent[128] = "\0";
  int i = 0;
  while (i < cpt)
  {
    sprintf(indent + i, "_");
    ++i;
  }
  printk("%s%s (pid=%d) %s, priority=%d\n",indent, task->comm ,task->pid, policy, priority);
  list_for_each(list, &task->children)
  {
    child = list_entry(list, struct task_struct, sibling);
    //recursive call
    getProcTree(child, cpt + 1);
  }
}

//Version 1
void getProcs(void)
{
  struct task_struct *task;
  int cpt = 0;
  for_each_process(task)
  {
    //Check policy and priority
    char policy[512];
    unsigned int policyInt = task->policy;
    int priority = 0;
    if (policyInt == SCHED_NORMAL)
    {
      sprintf(policy,"NORMAL");
      priority = task->prio;
    }
    else if (policyInt == SCHED_FIFO)
    {
      sprintf(policy,"FIFO");
      priority = task->rt_priority;
    }
    else if (policyInt == SCHED_BATCH)
    {
      sprintf(policy,"BATCH");
      priority = task->rt_priority;
    }
    else if (policyInt ==SCHED_RR)
    {
      sprintf(policy,"RR");
      priority = task->prio;
    }
    else
    {
      sprintf(policy,"IDLE");
      priority = task->prio;
    }
    printk("%s (pid=%d) %s, priority=%d\n",task->comm , task->pid, policy, priority);
    ++cpt;
  }
  printk("Number of tasks : %d\n", cpt);
}

static int my_show_function(struct file *file, char *buf, size_t count, loff_t *ppos)
{
  //Stop reading if the flag is set
  if (stop == 1)
  {
    kfree(tmpBuf);
    //put the initial values to obtain the real value for each cat
    tmpBuf = NULL;
    stop = 0;
    return 0;
  }
  //Create the message containing the proc data
  if (tmpBuf == NULL)
  {
    BUFFER_SIZE = count;
    tmpBuf = (char *) kmalloc(BUFFER_SIZE, GFP_KERNEL);
    getProcTreeForProc(&init_task, 0, tmpBuf);
  }
  //Select the size needed to copy in the user space
  int nbToCopy = 0;
  //if we are at the end of the message, just put the end
  //and set the flag
  if(count + *ppos > len)
  {
    nbToCopy = len - *ppos;
    stop = 1;
  }
  else
  {
    nbToCopy = count;
  }
  //copy in the user space
  if (copy_to_user(buf, tmpBuf + (*ppos), nbToCopy) != 0)
  {
    return -EFAULT;
  }
  //increment ppos
  *ppos += nbToCopy;
  return count;
}

static const struct file_operations fops = {
	.read = my_show_function
};

int init_module(void)
{
  struct proc_dir_entry * proc_entry;
	//Add entry prockernel in the proc directory
  proc_entry = proc_create("prockernel", 0444, NULL, &fops);
	if(proc_entry == NULL)
	{
		printk("prockernel: failed to create proc entry.\n");
		return -ENOMEM;
	}
  //Version 1
  //getProcs()
  //Version 2
  //getProcTree(&init_task, 0);

	printk("prockernel: entry created\n");
	return 0;
}

void cleanup_module(void)
{
  //Remove entry prockernel in directory
	remove_proc_entry("prockernel", NULL);
	printk("prockernel: entry deleted\n");
}

MODULE_LICENSE("GPL");
