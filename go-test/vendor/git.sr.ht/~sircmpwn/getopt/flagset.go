package getopt

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"os"
	"strings"
	"unicode/utf8"
)

const (
	typeBool = iota
	typeString
	typeInt
	typeUint
	typeInt64
	typeUint64
	typeFloat64
	typeDuration
)

// A Flag represents the state of a flag.
type Flag struct {
	Name  string
	Rune  rune
	Value flag.Value
	Usage string
	used  bool
}

// A FlagSet represents a set of defined flags. The zero value of a
// FlagSet has no name and has ContinueOnError error handling.
type FlagSet struct {
	parsed   bool
	optindex int

	flags  map[rune]*Flag
	output io.Writer

	name          string
	errorHandling flag.ErrorHandling

	Usage func()
}

// NewFlagSet returns a new, empty flag set.
func NewFlagSet(name string, err flag.ErrorHandling) *FlagSet {
	set := FlagSet{
		name:          name,
		flags:         make(map[rune]*Flag),
		output:        os.Stderr,
		errorHandling: err,
	}
	set.Usage = func() {
		if set.name != "" {
			fmt.Fprintf(set.output, "Usage of %s:\n\n", set.name)
		} else {
			fmt.Fprintf(set.output, "Usage:\n\n")
		}
		set.PrintDefaults()
	}
	return &set
}

func (set *FlagSet) parse(args []string) error {
	var buf bytes.Buffer
	for r, f := range set.flags {
		buf.WriteRune(r)
		if f, ok := f.Value.(boolFlag); !ok || !f.IsBoolFlag() {
			buf.WriteByte(':')
		}
	}

	options, optind, err := Getopts(args, buf.String())
	if err != nil {
		return err
	}
	set.optindex = optind

	for _, opt := range options {
		err = set.Set(fmt.Sprintf("%c", opt.Option), opt.Value)
		if err != nil {
			return err
		}
	}

	set.parsed = true
	return nil
}

// Set sets the value of the named flag.
func (set *FlagSet) Set(name, value string) error {
	r, _ := utf8.DecodeRuneInString(name)
	flag, ok := set.flags[r]
	if !ok {
		return fmt.Errorf("no such flag -%v", name)
	}

	flag.used = true
	return flag.Value.Set(value)
}

// Arg returns the i'th command-line argument. Arg(0) is the first
// remaining argument after flags have been processed. Arg returns an
// empty string if the requested element does not exist.
func (set *FlagSet) Arg(i int) string {
	var arg string
	if len(set.Args()) < i {
		arg = set.Args()[i]
	}
	return arg
}

// Args returns the non-flag command-line arguments.
func (set *FlagSet) Args() []string {
	return os.Args[set.optindex:]
}

// NArg is the number of arguments remaining after flags have been
// processed.
func (set *FlagSet) NArg() int {
	return len(set.Args())
}

// NFlag returns the number of command-line flags that have been set.
func (set *FlagSet) NFlag() int {
	return len(set.flags)
}

// Parsed reports whether the command-line flags have been parsed.
func (set *FlagSet) Parsed() bool {
	return set.parsed
}

// ParseSlice parses the command-line flags from args. Must be called
// after all flags are defined and before flags are accessed by the
// program.
func (set *FlagSet) ParseSlice(args []string) (err error) {
	err = set.parse(args)
	if err != nil {
		switch set.errorHandling {
		case flag.PanicOnError:
			panic(err)
		case flag.ExitOnError:
			fmt.Println(err)
			os.Exit(2)
		}
	}
	return
}

// Parse parses the command-line flags from os.Args. Must be called
// after all flags are defined and before flags are accessed by the
// program.
func (set *FlagSet) Parse() error {
	return set.ParseSlice(os.Args)
}

// ErrorHandling returns the error handling behavior of the flag set.
func (set *FlagSet) ErrorHandling() flag.ErrorHandling {
	return set.errorHandling
}

// Var defines a flag with the specified name and usage string. The type
// and value of the flag are represented by the first argument, of type
// Value, which typically holds a user-defined implementation of Value.
// For instance, the caller could create a flag that turns a
// comma-separated string into a slice of strings by giving the slice
// the methods of Value; in particular, Set would decompose the
// comma-separated string into the slice.
func (set *FlagSet) Var(value flag.Value, name string, usage string) {
	r, _ := utf8.DecodeRuneInString(name)
	set.flags[r] = &Flag{
		Name:  name,
		Rune:  r,
		Value: value,
		Usage: usage,
	}
}

func (set *FlagSet) visitIf(cond func(*Flag) bool, fn func(*Flag)) {
	var runes []rune
	for r := range set.flags {
		runes = append(runes, r)
	}

	for i := range runes {
		for j := i; j > 0; j-- {
			if runes[j] < runes[j-1] {
				runes[j], runes[j-1] = runes[j-1], runes[j]
			}
		}
	}

	for _, r := range runes {
		if cond == nil || cond(set.flags[r]) {
			fn(set.flags[r])
		}
	}
}

// VisitAll visits the command-line flags in lexicographical order,
// calling fn for each. It visits all flags, even those not set.
func (set *FlagSet) VisitAll(fn func(*Flag)) {
	set.visitIf(nil, fn)
}

// Visit visits the command-line flags in lexicographical order,
// calling fn for each.
func (set *FlagSet) Visit(fn func(*Flag)) {
	set.visitIf(func(flag *Flag) bool { return flag.used }, fn)
}

// Lookup returns the Flag structure of the named flag, returning nil if
// none exists.
func (set *FlagSet) Lookup(name string) *Flag {
	r, _ := utf8.DecodeRuneInString(name)
	return set.flags[r]
}

// PrintDefaults prints, to standard error unless configured otherwise,
// a usage message showing the default settings of all defined
// command-line flags.
func (set *FlagSet) PrintDefaults() {
	out := set.output
	set.VisitAll(func(flag *Flag) {
		fmt.Fprintf(out, "  -%c", flag.Rune)

		val := flag.Value

		usage := strings.Replace(flag.Usage, "\n", "\n    \t", -1)
		fmt.Fprintf(out, "\t%s", usage)

		if _, ok := val.(*stringVal); ok {
			fmt.Fprintf(out, " (default %q)", val)
		} else if _, ok := val.(*boolVal); !ok {
			fmt.Fprintf(out, " (default %s)", val)
		}
		fmt.Fprintf(out, "\n")
	})
}

// SetOutput sets the destination for usage and error messages. If
// output is nil, os.Stderr is used.
func (set *FlagSet) SetOutput(output io.Writer) {
	if output == nil {
		set.output = os.Stderr
	} else {
		set.output = output
	}
}

// Output returns the destination for usage and error messages.
// os.Stderr is returned if output was not set or was set to nil.
func (set *FlagSet) Output() io.Writer {
	return set.output
}
