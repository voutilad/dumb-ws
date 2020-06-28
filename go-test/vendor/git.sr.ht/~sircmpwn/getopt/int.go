package getopt

import (
	"fmt"
)

type intVal int

func (i *intVal) String() string {
	return fmt.Sprint(*i)
}

func (i *intVal) Set(val string) error {
	_, err := fmt.Sscanf(val, "%d", i)
	return err
}

// IntVar defines a int flag with specified name, default value,
// and usage string. The argument p points to a int variable in
// which to store the value of the flag.
func (set *FlagSet) IntVar(p *int, name string, value int, usage string) {
	*p = value
	set.Var((*intVal)(p), name, usage)
}

// Int defines an int flag with specified name, default value, and usage
// string. The return value is the address of an int variable that
// stores the value of the flag.
func (set *FlagSet) Int(name string, value int, usage string) *int {
	p := new(int)
	set.IntVar(p, name, value, usage)
	return p
}

type int64Val int64

func (i *int64Val) String() string {
	return fmt.Sprint(int64(*i))
}

func (i *int64Val) Set(val string) error {
	_, err := fmt.Sscanf(val, "%d", i)
	return err
}

// Int64Var defines an int64 flag with specified name, default value,
// and usage string. The argument p points to an int64 variable in which
// to store the value of the flag.
func (set *FlagSet) Int64Var(p *int64, name string, value int64, usage string) {
	*p = value
	set.Var((*int64Val)(p), name, usage)
}

// Int64 defines an int64 flag with specified name, default value, and
// usage string. The return value is the address of an int64 variable
// that stores the value of the flag.
func (set *FlagSet) Int64(name string, value int64, usage string) *int64 {
	var i int64
	set.Int64Var(&i, name, value, usage)
	return &i
}
