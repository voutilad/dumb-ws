# getopt [![godoc](https://godoc.org/git.sr.ht/~sircmpwn/getopt?status.svg)](https://godoc.org/git.sr.ht/~sircmpwn/getopt) [![builds.sr.ht status](https://builds.sr.ht/~sircmpwn/getopt.svg)](https://builds.sr.ht/~sircmpwn/getopt)

A POSIX-compatible getopt implementation for Go, because POSIX getopt is The
Correct Way to interpret arguments to command line utilities.

Please send patches/bugs/feedback to
[~sircmpwn/public-inbox@lists.sr.ht](https://lists.sr.ht/~sircmpwn/public-inbox).

## Example Usage

```go
import (
	"os"
	"git.sr.ht/~sircmpwn/getopt"
)

func main() {
	opts, optind, err := getopt.Getopts(os.Args, "abc:d:")
	if err != nil {
		panic(err)
	}
	for _, opt := range opts {
		switch opt.Option {
		case 'a':
			println("Option -a specified")
		case 'b':
			println("Option -b specified")
		case 'c':
			println("Option -c specified: " + opt.Value)
		case 'd':
			println("Option -d specified: " + opt.Value)
		}
	}
	println("Remaining arguments:")
	for _, arg := range os.Args[optind:] {
		println(arg)
	}
}
```

A [flag](https://golang.org/pkg/flag/)-like interface is also supported.

```go
import (
	"git.sr.ht/~sircmpwn/getopt"
)

func main() {
	a := getopt.Bool("a", false, "turn on option a")
	b := getopt.Int("b", 1, "set b to a numerical value")
	var opt string
	getopt.StringVar(&opt, "c", "", "let c be specified string")

	err := getopt.Parse()
	if err != nil {
		panic(err)
	}

	print("Value of a: ")
	println(*a)
	print("Value of b: ")
	println(*b)
	println("Value of c: " + opt)

	println("Remaining arguments:")
	for _, arg := range getopt.Args() {
		println(arg)
	}
}
```
