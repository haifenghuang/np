var cList = [];

for(var a = 1; a < 1000000; a++) {
	var newHash = { test : a };
	newHash.test2 = a*a;
	cList.push(newHash);
}