// $Id: Monitor_Base.inl 81691 2008-05-14 11:09:21Z johnnyw $

ACE_BEGIN_VERSIONED_NAMESPACE_DECL

namespace ACE
{
  namespace Monitor_Control
  {
    ACE_INLINE
    const char*
    Monitor_Base::name (void) const
    {
      return this->name_.c_str ();
    }

    ACE_INLINE
    void
    Monitor_Base::name (const char* new_name)
    {
      this->name_ = new_name;
    }
  }
}

ACE_END_VERSIONED_NAMESPACE_DECL

