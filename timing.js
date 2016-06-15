//
// Timing test
//

var a = [ ];
var n = 1000;
a.length = n;

var startTime = Date.now();

for (var i = 0; i < n; ++i) {
    for (var j = 0; j < n; ++j) {
        a[j] = j * (j + 1) / 2;
    }
}

var t = Date.now() - startTime;
print("Run time: " + (t) + "ms\n");
