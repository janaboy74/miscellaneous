#include <stdio.h>
#include <map>
#include <set>
#include <stack>

using namespace std;

unsigned char bmap[]={
  0b00000000,
  0b00000000,
  0b01111110,
  0b01000010,
  0b01000000,
  0b01101000,
  0b00111000,
  0b00000000
};

unsigned char bmap2[]={
  0,0,0,0,0,0,0,0
};

struct pos
{
  pos():x(0),y(0)
  {}
  pos(int x, int y)
  {
    this->x = x;
    this->y = y;
  }
  void operator = (const pos p)
  {
    x = p.x;
    y = p.y;
  }
  bool operator == (const pos p) const
  {
    return (x == p.x && y == p.y);
  }
  bool operator != (const pos p) const
  {
    return (x != p.x || y != p.y);
  }
  int operator < (const pos p) const
  {
    if (x < p.x)
      return true;
    if (x == p.x && y < p.y)
      return true;
    return false;
  }
  int x;
  int y;
};

struct node
{
  bool operator == (const node other) const
  {
    return p == other.p;
  }
  int operator < (const node other) const
  {
    return p < other.p;
  }
  pos p;
  int dist;
};

bool isvalid(pos p)
{
  if (p.x<0||p.x>7||p.y<0||p.y>7)
    return false;
  return true;
}

bool check(pos p)
{
  if (!isvalid(p))
    return true;
  return (bmap[p.y]>>(7-p.x))&1;
}

void add(pos actpos, int dist, map<pos, int> &done, set<node> &points)
{
  node cur;
  done[actpos] = dist;
  cur.p = actpos;
  cur.dist = dist;
  points.insert(cur);
}

pos getnext(pos p, int id)
{
  switch (id)
  {
  case 0:
    return pos(p.x-1,p.y);
  case 1:
    return pos(p.x+1,p.y);
  case 2:
    return pos(p.x,p.y-1);
  case 3:
    return pos(p.x,p.y+1);
  case 4:
    return pos(p.x-1,p.y-1);
  case 5:
    return pos(p.x+1,p.y-1);
  case 6:
    return pos(p.x-1,p.y+1);
  case 7:
    return pos(p.x+1,p.y+1);
  }
  return pos(0,0);
}

stack<node> calc(pos from, pos target)
{
  int directions = 4; // 4 or 8
  node start;
  start.p = from;
  start.dist = 0;
  set<node> points;
  map<pos, int> done;
  points.insert(start);
  done[from]=0;
  while( points.size() )
  {
    node first = *points.begin();
    pos actpos;
    points.erase(points.begin());
    for (int i=0;i<directions;++i)
    {
      actpos = getnext(first.p,i);
      if (!check(actpos))
      {
        if (done.find(actpos)==done.end())
        {
          add(actpos,first.dist+1, done, points);
        }
        else if (done[actpos]>first.dist+1)
        {
          add(actpos,first.dist+1, done, points);
        }
      }
    }
  }
  pos p = target;
  stack<node> waypoints;
  node n;
  n.p = p;
  n.dist = done[p];
  waypoints.push(n);
  while( p != from )
  {
    bmap2[p.y] |= 1<<(7-p.x);
    //printf("%d:%d - ", p.x, p.y);
    pos actpos;
    int dist = done[p] - 1;
    for (int i=0;i<directions;++i)
    {
      actpos = getnext(p,i);
      if (!isvalid(actpos)||done.find(actpos)==done.end())
      {
        continue;
      }
      if (done[actpos]==dist)
      {
        p = actpos;
        n.p = p;
        n.dist = done[p];
        waypoints.push(n);
        break;
      }
    }
  }
  printf("\n");
  for ( int y = 0;y < 8; ++y )
  {
    for ( int x = 0;x < 8; ++x )
    {
      if (check(pos(x,y)))
        printf("  #");
      else if (bmap2[y]>>(7-x)&1)
        printf("  X");
      else
        printf("%3d", done[pos(x,y)]);
    }
    printf("\n");
  }
  return waypoints;
}

int main(int argc, char ** argv)
{
  stack<node> nodes = calc(pos(4,4),pos(0,7));
  while( nodes.size() )
  {
    node vnode = nodes.top();
    nodes.pop();
    printf("node: %d:%d distance: %d\n", vnode.p.x, vnode.p.y, vnode.dist );
  }
}
