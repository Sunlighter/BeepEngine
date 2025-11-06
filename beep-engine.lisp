
; this is for LispWorks

(in-package :CL-USER)

(fli:register-module "Sunlighter.BeepEngine.dll" :file-name "Sunlighter.BeepEngine\\x64\\Debug\\Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (start-beep-engine "StartBeepEngine" :source) () :result-type :int-boolean :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (stop-beep-engine "StopBeepEngine" :source) () :result-type :void :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-running-p "IsBeepEngineRunning" :source) () :result-type :int-boolean :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-beep "BeepEngineBeep" :source) ((frequency :float) (duration :float)) :result-type :void :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-clear-buffer "BeepEngineClearBuffer" :source) () :result-type :void :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-add-note-to-buffer "BeepEngineAddNoteToBuffer" :source)
  ((start-time :float) (frequency :float) (amplitude :float) (duration :float))
  :result-type :void :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-add-event-to-buffer "BeepEngineAddEventToBuffer" :source)
  ((time :float) (event-id (:unsigned :int)))
  :result-type :void :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-start-play-buffer "BeepEngineStartPlayBuffer" :source) () :result-type :void :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(fli:define-foreign-function (beep-engine-wait-for-event "BeepEngineWaitForEvent" :source)
  ((event-id (:unsigned :int)))
  :result-type :bool :language :ansi-c :module "Sunlighter.BeepEngine.dll")

; (dotimes (i 500) (beep-engine-add-note-to-buffer (random 30.0) (* 55 (expt 2.0 (random 5.0))) 0.0625 1.0))

(fli:define-foreign-function (beep-engine-fft "FFT" :source)
  ((src (:pointer :float)) (dest (:pointer :float)) (size :int) (is-inverse :bool))
  :result-type :bool :language :ansi-c :module "Sunlighter.BeepEngine.dll")

(defun make-fft-vector (size)
  (make-array size :element-type 'single-float :initial-element 0.0s0))

(defun power-of-two-p (size)
  (and (integerp size) (> size 0) (= (logand size (- size 1)) 0)))

(defun fft (input-vector &key inverse)
  (if (not (vectorp input-vector)) (error "Input is not a vector"))
  (let (
      (size (length input-vector)))
    (if (not (power-of-two-p size)) (error "Input does not have power of two size"))
    (if (not (> size 2)) (error "Input must have size of at least 2"))
    (fli:with-dynamic-foreign-objects (
        (src :float :nelems size)
        (dest :float :nelems size))
      (dotimes (i size) (setf (fli:dereference src :index i) (aref input-vector i)))
      (if
        (beep-engine-fft src dest (/ size 2) inverse)
        (let* (
            (result (make-fft-vector size)))
          (dotimes (i size) (setf (aref result i) (fli:dereference dest :index i)))
          result)
        nil))))

#|

(setq buf (make-fft-vector 32))

(setf (aref buf 2) 1.0s0)

(fft buf :inverse t)

|#
