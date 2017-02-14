# shift

Returns the input array but shifted in one of the axes, by one or more chunk lengths. Syntax:
```
shift(input, dimension, length)
```

Where

 * input: any scidb array
 * dimension: the name of a dimension along which to shift
 * length: the number of positions to shift by, must be a multiple of chunk length along the dimension
 
For example, given a starting array like this:
```bash
$ iquery -aq "create array foo <val:double>[i=0:*,2,0, j=0:*,10,0]"
$ iquery -aq "store(build(<val:double>[i=0:1,2,0,j=0:9,10,0], i+j),foo)"
{i,j} val
{0,0} 0
{0,1} 1
{0,2} 2
{0,3} 3
{0,4} 4
{0,5} 5
{0,6} 6
{0,7} 7
{0,8} 8
{0,9} 9
{1,0} 1
{1,1} 2
{1,2} 3
{1,3} 4
{1,4} 5
{1,5} 6
{1,6} 7
{1,7} 8
{1,8} 9
{1,9} 10
```

We can append a copy of the data to itself, along `i` like this:
```bash
$ iquery -aq "insert(shift(foo, i, 2), foo)"
{i,j} val
{0,0} 0
{0,1} 1
{0,2} 2
{0,3} 3
{0,4} 4
{0,5} 5
{0,6} 6
{0,7} 7
{0,8} 8
{0,9} 9
{1,0} 1
{1,1} 2
{1,2} 3
{1,3} 4
{1,4} 5
{1,5} 6
{1,6} 7
{1,7} 8
{1,8} 9
{1,9} 10
{2,0} 0
{2,1} 1
{2,2} 2
{2,3} 3
{2,4} 4
{2,5} 5
{2,6} 6
{2,7} 7
{2,8} 8
{2,9} 9
{3,0} 1
{3,1} 2
{3,2} 3
{3,3} 4
{3,4} 5
{3,5} 6
{3,6} 7
{3,7} 8
{3,8} 9
{3,9} 10
```

Now pick up just the last two rows and append them again:
```
$ iquery -aq "insert(shift(between(foo,2,null,3,null),i,2),foo)"
{i,j} val
{0,0} 0
{0,1} 1
{0,2} 2
{0,3} 3
{0,4} 4
{0,5} 5
{0,6} 6
{0,7} 7
{0,8} 8
{0,9} 9
{1,0} 1
{1,1} 2
{1,2} 3
{1,3} 4
{1,4} 5
{1,5} 6
{1,6} 7
{1,7} 8
{1,8} 9
{1,9} 10
{2,0} 0
{2,1} 1
{2,2} 2
{2,3} 3
{2,4} 4
{2,5} 5
{2,6} 6
{2,7} 7
{2,8} 8
{2,9} 9
{3,0} 1
{3,1} 2
{3,2} 3
{3,3} 4
{3,4} 5
{3,5} 6
{3,6} 7
{3,7} 8
{3,8} 9
{3,9} 10
{4,0} 0
{4,1} 1
{4,2} 2
{4,3} 3
{4,4} 4
{4,5} 5
{4,6} 6
{4,7} 7
{4,8} 8
{4,9} 9
{5,0} 1
{5,1} 2
{5,2} 3
{5,3} 4
{5,4} 5
{5,5} 6
{5,6} 7
{5,7} 8
{5,8} 9
{5,9} 10
```

This is basically a rough toy, useful when you need to artificially inflate datasets.
