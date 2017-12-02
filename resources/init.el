(def {defn} (lambda {args body} {def (head args) (lambda (tail args) body)}))

(def {nil} {})

(defn {square x} {
      * x x})

(defn {first l} {
      eval (head l)})

(defn {second l} {
      first (tail l)})

(defn {last l} {
      if (== 1 (len l))
      {first l}
      {last (tail l)}})

(defn {apply f l} {
      eval (join (list f) l)})

(defn {fact n} {
      if (<= n 1)
        {1}
        {* n (fact (dec n))}})

(defn {reverse l} {
      if (== l {})
         {{}}
         {join (reverse (tail l)) (head l)}})

(defn {nth n l} {
      if (== n 0)
        {first l}
        {nth (dec n) (tail l)}})

;; Checks if element S is contained in list L.
(defn {contains? s l} {
      if (== 0 (len l))
        {false}
        {
          if (== s (first l))
          {true}
          {contains? s (tail l)}
        }})
