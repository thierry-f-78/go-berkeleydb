package berkeleydb

import "testing"
import "time"
import "sync"
import "os/exec"

func Test(t *testing.T) {
	var wg sync.WaitGroup
	db := Open("test-data/test.db")

	for i := 0; i < 3; i++ {
		go func ()() {
			for {
				println(db.Get("toto"))
				println(db.Get("toyto"))
				time.Sleep(100 * time.Millisecond)
			}
		}()
	}
	wg.Add(1)
	go func ()() {
		/* manipulate files */
		exec.Command("rm", "-f", "test-data/test.db").Run()
		time.Sleep(1100 * time.Millisecond)
		exec.Command("cp", "test-data/test_string.db", "test-data/test.db").Run()
		time.Sleep(1100 * time.Millisecond)
		exec.Command("cp", "test-data/test_string2.db", "test-data/test.db").Run()
		time.Sleep(1100 * time.Millisecond)
		exec.Command("cp", "test-data/test_string", "test-data/test").Run()
		exec.Command("postmap", "test-data/test").Run()
		time.Sleep(1100 * time.Millisecond)
		exec.Command("cp", "test-data/test_string2", "test-data/test").Run()
		exec.Command("postmap", "test-data/test").Run()
		time.Sleep(1100 * time.Millisecond)
		wg.Done()
	} ()
	wg.Wait()
}
