# -*- coding:utf-8 -*-
f=open('hash.txt')
s = f.readline()
ss=''''''
cnt=1
while cnt<200:
     t = ""
     if s =="end": break
     else:
          s= t.join(s.split('\n'))
          if len(s)==5:
               ss = ss + "|| ph=="+s
     s=f.readline()
     cnt = cnt+1

print ss
