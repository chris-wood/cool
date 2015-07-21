## open file
(open "/some/file/name.txt")

## print first line
(let ((in (open "/some/file/name.txt")))
  (format t "~a~%" (read-line in))
  (close in))

## print every line
(let ((in (open "/some/file/name.txt" :if-does-not-exist nil)))
  (when in
    (loop for line = (read-line in nil)
         while line do (format t "~a~%" line))
    (close in)))

