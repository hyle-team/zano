"""Actual core order/gate assertions and a finite publication-state model.
No SDK signing, wallet, transport, JNI or filesystem behavior is executed.
"""
from pathlib import Path
import sys,json
root=Path(sys.argv[1]);text=(root/'src/wallet/native_wallet_session.cpp').read_text()
def sign_body(source):
 lo=source.index('  file_session::sign_result file_session::sign_payment_to_held(')
 return source[lo:source.index('  payment_result file_session::assess_local_payment',lo)]
current=sign_body(text)
prior=sign_body((root/'.context/implementation/signed-prepared-publication/before/src/wallet/native_wallet_session.cpp').read_text())
events={'prepare':'m_wallet->prepare_transaction','inputs':'built->inputs.push_back(input)',
 'sign':'m_wallet->finalize_transaction','validate':'!currency::get_tx_fee(finalized.tx, actual_fee)',
 'reserve':'WALLET_TRANSFER_DETAIL_FLAG_COLD_SIG_RESERVATION','render':'m_wallet->render_native_checkpoint_image',
 'prepare_record':'m_material->prepare_signed_payment','replace':'m_material->replace_signed_checkpoint',
 'complete':'m_material->complete_signed_payment'}
pos={name:current.index(token) for name,token in events.items()}
assert prior.index('m_material->begin_signed_payment')<prior.index(events['prepare_record']), 'accepted r3 two-file gap reproduced'
assert 'm_material->begin_signed_payment' not in current
assert list(pos.values())==sorted(pos.values())
assert current.count(events['prepare_record'])==1
assert current.index('built->generation = m_generation; built->revision = m_revision;')<current.index('++m_revision')<pos['prepare_record']
assert 'construction_entered = true;' in current[pos['inputs']:pos['sign']]
assert 'publication_boundary_entered = true;' in current[pos['render']:pos['prepare_record']]
guard='if (stop.load() || !admission.current() || !authorized())'
assert guard in current[pos['render']:pos['prepare_record']]
assert 'mark_recovery(); return refuse(sign_status::Revoked);' in current[pos['render']:pos['prepare_record']]
assert 'mark_recovery(); return refuse(sign_status::RecoveryRequired);' in current[pos['validate']:pos['reserve']]
assert 'mark_recovery(); return refuse(sign_status::RecoveryRequired);' in current[pos['render']:pos['prepare_record']]
catch=current[current.rindex('} catch (...)'):]
assert 'if (!construction_entered && !publication_boundary_entered)' in catch
assert catch.index('return refuse(sign_status::PreparationFailed)')<catch.index('mark_recovery()')
assert 'finalize_transaction(ftp, finalized, false, false)' in current
assert 'next authenticated Open' in current
start=text.index('  void file_session::mark_recovery()');marked=text[start:text.index('\n  }',start)+4]
assert 'm_read_only_sync_faulted = true' in marked and 'm_state = session_state::RecoveryRequired' in marked
for name,token in [('checkpoint','  checkpoint_result file_session::write_checkpoint()'),('close','  close_result file_session::close()')]:
 lo=text.index(token);portion=text[lo:text.index('\n  }',lo)+4]
 assert 'case session_state::RecoveryRequired: return '+name+'_result::RecoveryRequired;' in portion
# Model the source's sticky refusal, preserving prior dirty/unchanged state only
# on definitely pre-construction failure. The provider is tested separately.
for state in ['OpenUnchanged','OpenDirty']:
 for phase in ['preparation','construction','validation','reservation','render','publication']:
  constructed=phase!='preparation';entered=phase=='publication'
  memory_state='RecoveryRequired' if constructed or entered else state
  checkpoint_allowed=memory_state in ['OpenUnchanged','OpenDirty']
  assert checkpoint_allowed==(phase=='preparation')
print(json.dumps({'result':'PASS','scope':'actual source-order/sticky-gate assertions and finite model only',
 'accepted_r3_marker_before_prepared':'reproduced','single_complete_prepared_boundary':'PASS',
 'captured_operation_identity':'PASS','unjournaled_constructed_state_cannot_checkpoint':'PASS',
 'native_runtime_execution':False}))
