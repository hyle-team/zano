import { Component, OnInit, NgZone, OnDestroy } from '@angular/core';
import { FormGroup, FormControl, Validators } from '@angular/forms';
import { BackendService } from '../_helpers/services/backend.service';
import { VariablesService } from '../_helpers/services/variables.service';
import { ModalService } from '../_helpers/services/modal.service';
import { Location } from '@angular/common';
import { ActivatedRoute } from '@angular/router';

@Component({
  selector: 'app-add-contacts',
  templateUrl: './add-contacts.component.html',
  styleUrls: ['./add-contacts.component.scss']
})
export class AddContactsComponent implements OnInit, OnDestroy {
  id: number;
  queryRouting;
  addContactForm = new FormGroup({
    address: new FormControl('', [
      Validators.required,
      (g: FormControl) => {
        if (g.value) {
          this.backend.validateAddress(g.value, valid_status => {
            this.ngZone.run(() => {
              if (valid_status === false) {
                g.setErrors(
                  Object.assign({ address_not_valid: true }, g.errors)
                );
              } else {
                if (g.hasError('address_not_valid')) {
                  delete g.errors['address_not_valid'];
                  if (Object.keys(g.errors).length === 0) {
                    g.setErrors(null);
                  }
                }
              }
            });
          });
          return g.hasError('address_not_valid')
            ? { address_not_valid: true }
            : null;
        }
        return null;
      },
      (g: FormControl) => {
        const isDublicated = this.variablesService.contacts.findIndex(
          contact => contact.address === g.value
        );
        if (isDublicated !== -1 && !(this.id === isDublicated)) {
          return { dublicated: true };
        }
        return null;
      }
    ]),
    notes: new FormControl('', [
      (g: FormControl) => {
        if (g.value) {
          if (g.value.length > this.variablesService.maxCommentLength) {
            return { maxLength: true };
          } else {
            return null;
          }
        } else {
          return null;
        }
      }
    ]),
    name: new FormControl('', [
      Validators.required,
<<<<<<< HEAD
<<<<<<< HEAD
      Validators.minLength(4),
      Validators.maxLength(25),
=======
      Validators.pattern(/^[\w\s-_.]{4,25}$/),
>>>>>>> contact service
=======
      Validators.minLength(4),
      Validators.maxLength(25),
>>>>>>> allow symbols at name field diffrent from latin
      (g: FormControl) => {
        if (g.value) {
          const isDublicated = this.variablesService.contacts.findIndex(
            contact => contact.name === g.value.trim()
          );
          if (isDublicated !== -1 && !(this.id === isDublicated)) {
            return { dublicated: true };
          }
          return null;
        }
      }
    ])
  });

  constructor(
    private route: ActivatedRoute,
    private backend: BackendService,
    public variablesService: VariablesService,
    private modalService: ModalService,
    private ngZone: NgZone,
    private location: Location
  ) {}

  ngOnInit() {
    this.queryRouting = this.route.queryParams.subscribe(params => {
      if (params.id) {
        this.id = parseInt(params.id, 10);
        this.addContactForm.reset({
          name: this.variablesService.contacts[params.id]['name'],
          address: this.variablesService.contacts[params.id]['address'],
          notes: this.variablesService.contacts[params.id]['notes']
        });
      } else {
        this.addContactForm.reset({
          name: this.variablesService.newContact['name'],
          address: this.variablesService.newContact['address'],
          notes: this.variablesService.newContact['notes']
        });
      }
    });
  }

  add() {
    if (!this.variablesService.appPass) {
      this.modalService.prepareModal(
        'error',
        'CONTACTS.FORM_ERRORS.SET_MASTER_PASSWORD'
      );
    } else {
      if (this.addContactForm.valid) {
        this.backend.validateAddress(
          this.addContactForm.get('address').value,
          valid_status => {
            if (valid_status === false) {
              this.ngZone.run(() => {
                this.addContactForm
                  .get('address')
                  .setErrors({ address_not_valid: true });
              });
            } else {
              if (this.id || this.id === 0) {
                this.variablesService.contacts.forEach((contact, index) => {
                  if (index === this.id) {
                    contact.name = this.addContactForm.get('name').value.trim();
                    contact.address = this.addContactForm.get('address').value;
                    contact.notes =
                      this.addContactForm.get('notes').value || '';
                  }
                });
                this.backend.storeSecureAppData();
                this.backend.getContactAlias();
                this.modalService.prepareModal(
                  'success',
                  'CONTACTS.SUCCESS_SAVE'
                );
              } else {
                this.variablesService.contacts.push({
                  name: this.addContactForm.get('name').value.trim(),
                  address: this.addContactForm.get('address').value,
                  notes: this.addContactForm.get('notes').value || ''
                });
                this.backend.storeSecureAppData();
                this.backend.getContactAlias();
                this.modalService.prepareModal(
                  'success',
                  'CONTACTS.SUCCESS_SENT'
                );
                this.variablesService.newContact = {
                  name: null,
                  address: null,
                  notes: null
                };
                this.addContactForm.reset({
                  name: null,
                  address: null,
                  notes: null
                });
              }
            }
          }
        );
      }
    }
  }

  back() {
    this.location.back();
  }

  ngOnDestroy() {
<<<<<<< HEAD
<<<<<<< HEAD
    if (!(this.id || this.id === 0)) {
=======
    if (!this.id) {
>>>>>>> contact service
=======
    if (!(this.id || this.id === 0)) {
>>>>>>> fix add contact + rebuild html
      this.variablesService.newContact = {
        name: this.addContactForm.get('name').value,
        address: this.addContactForm.get('address').value,
        notes: this.addContactForm.get('notes').value
      };
    }
    this.queryRouting.unsubscribe();
  }
}
