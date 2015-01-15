# -*- coding:utf-8 -*-
f=open('hash.txt')
s = f.readline()
ss=''''''
while True:
     t = ""
     if s =="end": break
     else:
          s= t.join(s.split('\n'))
          if len(s)==5:
               ss = ss + "|| ph=="+s
     s=f.readline()

print ss
