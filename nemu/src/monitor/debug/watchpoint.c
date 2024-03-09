#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

static int next_No;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
    wp_pool[i].value=0;
    wp_pool[i].count=0;
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
  next_No = 0;
}

WP* new_wp(char* args){
  if (free_==NULL)
  {
    printf("Error: No watchpoint free!\n");
    assert(0);
  } 
  WP* new_wp = free_;
  free_ = free_->next;
  new_wp -> NO = next_No;
  next_No += 1;
  new_wp -> next = NULL;
  strcpy(new_wp->expr,args);
  new_wp->count=0;
  bool success = true;
  new_wp->value=expr(new_wp->expr,&success);
  if(success==false)
  {
    printf("Error: Expr() fault in new_wq.\n");
    return false;
  }
  if(head==NULL)
  {
    head = new_wp;
  }//test
  else
  {
    new_wp -> next = head;
    head = new_wp;
  }
  return new_wp;
}

void free_wp(WP* wp){
  if(head == NULL)
  {
    printf("Error: No watchpoint.\n");
    assert(0);
  }
  if(head == wp)
  {
    head=head->next;
  }
  else
  {
    WP* tmp_wp = head;
    while(tmp_wp->next!=NULL)
    {
      if(tmp_wp -> next == wp)
      {
        tmp_wp -> next = wp -> next;
        break;
      }
    }
  }
  wp -> next = free_;
  free_ = wp;
}

void print_wp()
{
  if(head==NULL)
  {
    printf("Error: No watchpoint.\n");
    return;
  }
  printf("Watchpoint:\n");
  printf("NO. Expr Count_Hit\n");
  WP* tmp_wp = head;
  while(tmp_wp != NULL)
  {
    printf("%d %s %d\n",tmp_wp -> NO,tmp_wp -> expr,tmp_wp -> count);
    tmp_wp = tmp_wp -> next;
  }
}

bool check_wp()
{
  bool success = true;
  int new_value;
  if(head == NULL)
  {
    return false;
  }
  WP* tmp_wp = head;
  while(tmp_wp != NULL)
  {
    new_value = expr(tmp_wp->expr,&success);
    if(new_value != tmp_wp -> value)
    {
      tmp_wp -> count++;
      printf("Hit watchpoint %d, expr: %s.\told value: %d -> new value: %d\n",tmp_wp->NO,tmp_wp->expr,tmp_wp->value,new_value);
      tmp_wp -> value = new_value;
      return true;
    }
    tmp_wp = tmp_wp->next;
  }
  return false;
}

WP* find_point(int num) 
{
  for(WP* tmp = head;tmp != NULL;tmp = tmp->next)
  {
    if(tmp->NO == num)
    {
      return tmp;
    }
  }
  return NULL;
}

/* TODO: Implement the functionality of watchpoint */
//todo

