package getopt

import (
	"flag"
	"io"
	"os"
	"time"
)

// CommandLine is the default set of command-line flags, parsed from
// os.Args. The top-level functions such as BoolVar, Arg, and so on are
// wrappers for the methods of CommandLine.
var CommandLine = NewFlagSet(os.Args[0], flag.ExitOnError)

// Usage prints a usage message documenting all defined command-line
// flags to os.Stderr. It is called when an error occurs while parsing
// flags. The function is a variable that may be changed to point to a
// custom function. By default it prints a simple header and calls
// PrintDefaults; for details about the format of the output and how to
// control it, see the documentation for PrintDefaults. Custom usage
// functions may choose to exit the program; by default exiting happens
// anyway as the command line's error handling strategy is set to
// ExitOnError.
var Usage = CommandLine.Usage

// PrintDefaults prints, to standard error unless configured otherwise,
// a usage message showing the default settings of all defined
// command-line flags.
func PrintDefaults() { CommandLine.PrintDefaults() }

// Arg returns the i'th command-line argument. Arg(0) is the first
// remaining argument after flags have been processed. Arg returns an
// empty string if the requested element does not exist.
func Arg(i int) string { return CommandLine.Arg(i) }

// Args returns the non-flag command-line arguments.
func Args() []string { return CommandLine.Args() }

// NArg is the number of arguments remaining after flags have been
// processed.
func NArg() int { return CommandLine.NArg() }

// NFlag returns the number of command-line flags that have been set.
func NFlag() int { return CommandLine.NFlag() }

// Parsed reports whether the command-line flags have been parsed.
func Parsed() bool { return CommandLine.Parsed() }

// Parse parses the command-line flags from os.Args. Must be called
// after all flags are defined and before flags are accessed by the
// program.
func Parse() error { return CommandLine.Parse() }

// Var defines a flag with the specified name and usage string. The type
// and value of the flag are represented by the first argument, of type
// Value, which typically holds a user-defined implementation of Value.
// For instance, the caller could create a flag that turns a
// comma-separated string into a slice of strings by giving the slice
// the methods of Value; in particular, Set would decompose the
// comma-separated string into the slice.
func Var(value flag.Value, name string, usage string) {
	CommandLine.Var(value, name, usage)
}

// Lookup returns the Flag structure of the named flag, returning nil if
// none exists.
func Lookup(name string) *Flag {
	return CommandLine.Lookup(name)
}

// SetOutput sets the destination for usage and error messages. If
// output is nil, os.Stderr is used.
func SetOutput(output io.Writer) {
	CommandLine.SetOutput(output)
}

// BoolVar defines a bool flag with specified name, default value, and
// usage string. The argument p points to a bool variable in which to
// store the value of the flag.
func BoolVar(p *bool, name string, value bool, usage string) {
	CommandLine.BoolVar(p, name, value, usage)
}

// Bool defines a bool flag with specified name, default value, and
// usage string. The return value is the address of a bool variable that
// stores the value of the flag.
func Bool(name string, value bool, usage string) *bool {
	return CommandLine.Bool(name, value, usage)
}

// DurationVar defines a time.Duration flag with specified name, default
// value, and usage string. The argument p points to a time.Duration
// variable in which to store the value of the flag. The flag accepts a
// value acceptable to time.ParseDuration.
func DurationVar(p *time.Duration, name string, value time.Duration, usage string) {
	CommandLine.DurationVar(p, name, value, usage)
}

// Duration defines a time.Duration flag with specified name, default
// value, and usage string. The return value is the address of a
// time.Duration variable that stores the value of the flag. The flag
// accepts a value acceptable to time.ParseDuration.
func Duration(name string, value time.Duration, usage string) *time.Duration {
	return CommandLine.Duration(name, value, usage)
}

// Float64Var defines a float64 flag with specified name, default value,
// and usage string. The argument p points to a float64 variable in
// which to store the value of the flag.
func Float64Var(p *float64, name string, value float64, usage string) {
	CommandLine.Float64Var(p, name, value, usage)
}

// Float64 defines a float64 flag with specified name, default value,
// and usage string. The return value is the address of a float64
// variable that stores the value of the flag.
func Float64(name string, value float64, usage string) *float64 {
	return CommandLine.Float64(name, value, usage)
}

// IntVar defines a int flag with specified name, default value,
// and usage string. The argument p points to a int variable in
// which to store the value of the flag.
func IntVar(p *int, name string, value int, usage string) {
	CommandLine.IntVar(p, name, value, usage)
}

// Int defines an int flag with specified name, default value, and usage
// string. The return value is the address of an int variable that
// stores the value of the flag.
func Int(name string, value int, usage string) *int {
	return CommandLine.Int(name, value, usage)
}

// Int64Var defines an int64 flag with specified name, default value,
// and usage string. The argument p points to an int64 variable in which
// to store the value of the flag.
func Int64Var(p *int64, name string, value int64, usage string) {
	CommandLine.Int64Var(p, name, value, usage)
}

// Int64 defines an int64 flag with specified name, default value, and
// usage string. The return value is the address of an int64 variable
// that stores the value of the flag.
func Int64(name string, value int64, usage string) *int64 {
	return CommandLine.Int64(name, value, usage)
}

// StringVar defines a string flag with specified name, default value,
// and usage string. The argument p points to a string variable in which
// to store the value of the flag.
func StringVar(p *string, name string, value string, usage string) {
	CommandLine.StringVar(p, name, value, usage)
}

// String defines a string flag with specified name, default value, and
// usage string. The return value is the address of a string variable
// that stores the value of the flag.
func String(name string, value string, usage string) *string {
	return CommandLine.String(name, value, usage)
}

// Uint64Var defines a uint64 flag with specified name, default value,
// and usage string. The argument p points to a uint64 variable in which
// to store the value of the flag.
func Uint64Var(p *uint64, name string, value uint64, usage string) {
	CommandLine.Uint64Var(p, name, value, usage)
}

// Uint64 defines a uint64 flag with specified name, default value, and
// usage string. The return value is the address of a uint64 variable
// that stores the value of the flag.
func Uint64(name string, value uint64, usage string) *uint64 {
	return CommandLine.Uint64(name, value, usage)
}

// Set sets the value of the named flag.
func Set(name, value string) error {
	return CommandLine.Set(name, value)
}
