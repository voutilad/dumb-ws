package getopt

import (
	"time"
)

type durationVal time.Duration

func (d *durationVal) String() string {
	return time.Duration(*d).String()
}

func (d *durationVal) Set(val string) error {
	v, err := time.ParseDuration(val)
	if err != nil {
		return err
	}
	*d = durationVal(v)
	return nil
}

// DurationVar defines a time.Duration flag with specified name, default
// value, and usage string. The argument p points to a time.Duration
// variable in which to store the value of the flag. The flag accepts a
// value acceptable to time.ParseDuration.
func (set *FlagSet) DurationVar(p *time.Duration, name string, value time.Duration, usage string) {
	*p = value
	set.Var((*durationVal)(p), name, usage)
}

// Duration defines a time.Duration flag with specified name, default
// value, and usage string. The return value is the address of a
// time.Duration variable that stores the value of the flag. The flag
// accepts a value acceptable to time.ParseDuration.
func (set *FlagSet) Duration(name string, value time.Duration, usage string) *time.Duration {
	var d time.Duration
	set.DurationVar(&d, name, value, usage)
	return &d
}
