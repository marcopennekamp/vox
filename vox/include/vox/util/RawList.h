#ifndef VOX_UTILS_RAWLIST_H_
#define VOX_UTILS_RAWLIST_H_

#include <stdlib.h>


namespace vox {

template <typename T>
class RawList {
private:
    T* data_;
    size_t iterator_;
    size_t size_;

    void CheckAllocationError () {
        if (data_ == NULL) {
            printf ("Error: RawList could not allocate '%llu' bytes (data pointer is NULL)!\n", size_);
        }
    }


public:
    RawList (size_t initial_size = 8) {
        size_ = initial_size;
        data_ = (T*) malloc (size_ * sizeof (T));
        CheckAllocationError ();
        ResetIterator ();
    }

    void Resize (size_t new_size) {
        const float kResizeTreshold = 0.2f;

        /* Resize by at least 20%. */
        bool resize = false;
        if (new_size > size_) {
            new_size = max (new_size, size_ + (size_t) (size_ * kResizeTreshold));
            resize = true;
        }else if (new_size < size_) {
            resize = true;
        }
        
        if (resize) {
            size_ = new_size;
            data_ = (T*) realloc (data_, size_ * sizeof (T));
            CheckAllocationError ();
        }
    }

    inline T& operator[] (size_t index) {
        return data_[index];
    }

    inline T& Next () {
        T& next = data_[iterator_];
        ++iterator_;
        return next;
    }

    inline void ResetIterator () {
        iterator_ = 0;
    }
    
    inline T* data () const { return data_; }
    inline const size_t iterator () { return iterator_; }
    inline const size_t size () const { return size_; }


};

}


#endif  /* VOX_UTILS_RAWLIST_H_ */