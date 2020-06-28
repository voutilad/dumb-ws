package getopt

import "fmt"

type uintVal uint

func (i *uintVal) String() string {
	return fmt.Sprint(uint(*i))
}

func (i *uintVal) Set(val string) error {
	_, err := fmt.Sscanf(val, "%d", i)
	return err
}

// UintVar defines a uint flag with specified name, default value, and
// usage string. The argument p points to a uint variable in which to
// store the value of the flag.
func (set *FlagSet) UintVar(p *uint, name string, value uint, usage string) {
	*p = value
	set.Var((*uintVal)(p), name, usage)
}

// UintVar defines a uint flag with specified name, default value, and
// usage string. The argument p points to a uint variable in which to
// store the value of the flag.
func UintVar(p *uint, name string, value uint, usage string) {
	CommandLine.UintVar(p, name, value, usage)
}

// Uint defines a uint flag with specified name, default value, and
// usage string. The return value is the address of a uint variable that
// stores the value of the flag.
func (set *FlagSet) Uint(name string, value uint, usage string) *uint {
	var b uint
	set.UintVar(&b, name, value, usage)
	return &b
}

// Uint defines a uint flag with specified name, default value, and
// usage string. The return value is the address of a uint variable that
// stores the value of the flag.
func Uint(name string, value uint, usage string) *uint {
	return CommandLine.Uint(name, value, usage)
}

type uint64Val uint64

func (i *uint64Val) String() string {
	return fmt.Sprint(uint64(*i))
}

func (i *uint64Val) Set(val string) error {
	_, err := fmt.Sscanf(val, "%d", i)
	return err
}

// Uint64Var defines a uint64 flag with specified name, default value,
// and usage string. The argument p points to a uint64 variable in which
// to store the value of the flag.
func (set *FlagSet) Uint64Var(p *uint64, name string, value uint64, usage string) {
	*p = value
	set.Var((*uint64Val)(p), name, usage)
}

// Uint64 defines a uint64 flag with specified name, default value, and
// usage string. The return value is the address of a uint64 variable
// that stores the value of the flag.
func (set *FlagSet) Uint64(name string, value uint64, usage string) *uint64 {
	var b uint64
	set.Uint64Var(&b, name, value, usage)
	return &b
}
