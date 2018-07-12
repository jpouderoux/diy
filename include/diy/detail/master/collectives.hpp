namespace diy
{
    namespace detail
    {
        struct CollectiveOp
        {
          virtual void    init()                                  =0;
          virtual void    update(const CollectiveOp& other)       =0;
          virtual void    global(const mpi::communicator& comm)   =0;
          virtual void    copy_from(const CollectiveOp& other)    =0;
          virtual void    result_out(void* dest) const            =0;
          virtual         ~CollectiveOp()                         {}
        };

        template<class T, class Op>
        struct AllReduceOp: public CollectiveOp
        {
                AllReduceOp(const T& x, Op op):
                  in_(x), op_(op)         {}

          void  init()                                    { out_ = in_; }
          void  update(const CollectiveOp& other)         { out_ = op_(out_, static_cast<const AllReduceOp&>(other).in_); }
          void  global(const mpi::communicator& comm)     { T res; mpi::all_reduce(comm, out_, res, op_); out_ = res; }
          void  copy_from(const CollectiveOp& other)      { out_ = static_cast<const AllReduceOp&>(other).out_; }
          void  result_out(void* dest) const              { *reinterpret_cast<T*>(dest) = out_; }

          private:
            T     in_, out_;
            Op    op_;
        };

        template<class T>
        struct Scratch: public CollectiveOp
        {
                Scratch(const T& x):
                  x_(x)                                   {}

          void  init()                                    {}
          void  update(const CollectiveOp&)               {}
          void  global(const mpi::communicator&)          {}
          void  copy_from(const CollectiveOp&)            {}
          void  result_out(void* dest) const              { *reinterpret_cast<T*>(dest) = x_; }

          private:
            T     x_;
        };
    }

    struct Master::Collective
    {
                    Collective():
                      cop_(0)                           {}
                    Collective(detail::CollectiveOp* cop):
                      cop_(cop)                         {}
                    Collective(Collective&& other):
                      cop_(0)                           { swap(const_cast<Collective&>(other)); }
                    ~Collective()                       { delete cop_; }

        Collective& operator=(const Collective& other)  = delete;
                    Collective(Collective& other)       = delete;

        void        init()                              { cop_->init(); }
        void        swap(Collective& other)             { std::swap(cop_, other.cop_); }
        void        update(const Collective& other)     { cop_->update(*other.cop_); }
        void        global(const mpi::communicator& c)  { cop_->global(c); }
        void        copy_from(Collective& other) const  { cop_->copy_from(*other.cop_); }
        void        result_out(void* x) const           { cop_->result_out(x); }

        detail::CollectiveOp*                           cop_;
    };

    struct Master::CollectivesList: public std::list<Collective>
    {};

    struct Master::CollectivesMap: public std::map<int, CollectivesList>
    {};
}

diy::Master::CollectivesMap&
diy::Master::
collectives()
{
    return *collectives_;
}

diy::Master::CollectivesList&
diy::Master::
collectives(int gid__)
{
    return (*collectives_)[gid__];
}
