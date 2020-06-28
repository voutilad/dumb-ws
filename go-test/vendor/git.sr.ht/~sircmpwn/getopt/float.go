package getopt

import (
	"fmt"
)

type floatVal float64

func (f *floatVal) String() string {
	return fmt.Sprint(float64(*f))
}

func (f *floatVal) Set(val string) error {
	_, err := fmt.Sscanf(val, "%g", f)
	return err
}

// Float64Var defines a float64 flag with specified name, default value,
// and usage string. The argument p points to a float64 variable in
// which to store the value of the flag.
func (set *FlagSet) Float64Var(p *float64, name string, value float64, usage string) {
	*p = value
	set.Var((*floatVal)(p), name, usage)
}

// Float64 defines a float64 flag with specified name, default value,
// and usage string. The return value is the address of a float64
// variable that stores the value of the flag.
func (set *FlagSet) Float64(name string, value float64, usage string) *float64 {
	var f float64
	set.Float64Var(&f, name, value, usage)
	return &f
}
