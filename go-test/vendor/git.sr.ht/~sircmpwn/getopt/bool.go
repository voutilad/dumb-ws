package getopt

import (
	"flag"
	"strconv"
)

type boolFlag interface {
	flag.Value
	IsBoolFlag() bool
}

type boolVal bool

func (b *boolVal) String() string {
	return strconv.FormatBool(bool(*b))
}

func (b *boolVal) Set(val string) error {
	*b = boolVal(true)
	return nil
}

func (b *boolVal) IsBoolFlag() bool {
	return true
}

// BoolVar defines a bool flag with specified name, default value, and
// usage string. The argument p points to a bool variable in which to
// store the value of the flag.
func (set *FlagSet) BoolVar(p *bool, name string, value bool, usage string) {
	*p = value
	set.Var((*boolVal)(p), name, usage)
}

// Bool defines a bool flag with specified name, default value, and
// usage string. The return value is the address of a bool variable that
// stores the value of the flag.
func (set *FlagSet) Bool(name string, value bool, usage string) *bool {
	var b bool
	set.BoolVar(&b, name, value, usage)
	return &b
}
