new myvar = '123'

print('multiline 
  string ' myvar 1) 
  
new mt = (: 1 2 3 a = 'b' c = 'd' (6) tb2 = (: 1 2 3 tbl2 = (: 10 11 12)) )

events(mt, (: 
	update = {(t key val) print('---update of key'  key val) }
	index = {(t key) print('---index of key'  key ) }
  ))

print('events of mt' events(mt) mt2)

mt.bla = 123;

print( mt[nil] )

print('table size: '..#mt..' size: '..size(mt)..' asize: '..asize(mt))

for i,v in each(mt) {
	print('each: '..i..' == '..tostring(v))
}

--[[with each(mt) i,v do
	print('with-each: '..i..' == '..v)
end]]--

if (1 == 2) {
	print('1 != 2 operator')
cond(1 == 1)
	print('nope')
else
	print('bla')
}

new obj = (:)
obj.v = 10
obj.test = {(a b) 
	print('object test: '..a[1]..' opt:'..a.opt..' sz:'..#a..' b:'..b)
}

new mfunc = {()};


new myfunc = {(a) return a, 2 }

obj.test((: 1 opt = 23) 1000+myfunc(3))

x = 3
new i = 1        -- local to the chunk

while i<=x {
  new x = i*2    -- local to the while body
  print(x)         --> 2, 4, 6, 8, ...
  i = i + 1
}