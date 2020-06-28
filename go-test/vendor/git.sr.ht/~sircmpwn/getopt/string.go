package getopt

type stringVal string

func (s *stringVal) String() string {
	return string(*s)
}

func (s *stringVal) Set(val string) error {
	*s = stringVal(val)
	return nil
}

// StringVar defines a string flag with specified name, default value,
// and usage string. The argument p points to a string variable in which
// to store the value of the flag.
func (set *FlagSet) StringVar(p *string, name string, value string, usage string) {
	*p = value
	set.Var((*stringVal)(p), name, usage)
}

// String defines a string flag with specified name, default value, and
// usage string. The return value is the address of a string variable
// that stores the value of the flag.
func (set *FlagSet) String(name string, value string, usage string) *string {
	var s string
	set.StringVar(&s, name, value, usage)
	return &s
}
