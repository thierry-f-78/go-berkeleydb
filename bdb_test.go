package berkeleydb

import "testing"
import "time"
import "sync"

func Test(t *testing.T) {
	var wg sync.WaitGroup
	db := Open("test-data/test_string.db")

	for i := 0; i < 10; i++ {
		wg.Add(1)
		go func ()() {
			for {
				println(db.Get("toto"))
				println(db.Get("toyto"))
				time.Sleep(5 * time.Millisecond)
			}
			wg.Done()
		}()
	}
	wg.Wait()
}
