<<<<<<< HEAD
import { Component, OnInit, NgZone } from '@angular/core';
=======
import { Component, OnInit } from '@angular/core';
>>>>>>> contact service
import { Location } from '@angular/common';
import { BackendService } from '../_helpers/services/backend.service';
import { VariablesService } from '../_helpers/services/variables.service';
import { Contact } from '../_helpers/models/contact.model';
import { ModalService } from '../_helpers/services/modal.service';
import { Papa } from 'ngx-papaparse';
import { TranslateService } from '@ngx-translate/core';
<<<<<<< HEAD
import { Router } from '@angular/router';
=======
>>>>>>> contact service

@Component({
  selector: 'app-export-import',
  templateUrl: './export-import.component.html',
  styleUrls: ['./export-import.component.scss']
})
export class ExportImportComponent implements OnInit {
  csvContent;

  constructor(
    private location: Location,
    private variablesService: VariablesService,
    private backend: BackendService,
    private modalService: ModalService,
    private papa: Papa,
<<<<<<< HEAD
    private translate: TranslateService,
    private router: Router,
    private ngZone: NgZone
=======
    private translate: TranslateService
>>>>>>> contact service
  ) {}

  ngOnInit() {}

  import() {
    this.backend.openFileDialog(
      '',
      '*',
      this.variablesService.settings.default_path,
      (file_status, file_data) => {
        if (file_status) {
          this.variablesService.settings.default_path = file_data.path.substr(
            0,
            file_data.path.lastIndexOf('/')
          );
          if (this.isValid(file_data.path)) {
            this.backend.loadFile(file_data.path, (status, data) => {
<<<<<<< HEAD
              if (!status) {
                this.modalService.prepareModal(
                  'error',
                  'CONTACTS.ERROR_IMPORT_EMPTY'
                );
              } else {
=======
              if (status) {
>>>>>>> contact service
                const options = {
                  header: true
                };
                const elements = this.papa.parse(data, options);
<<<<<<< HEAD
                const isArray = Array.isArray(elements.data);
                if (isArray && elements.data.length !== 0 && !elements.errors.length) {
=======

                if (elements.data && !elements.errors.length) {
>>>>>>> contact service
                  if (!this.variablesService.contacts.length) {
                    elements.data.forEach(element => {
                      this.variablesService.contacts.push(element);
                    });
                  } else {
                    elements.data.forEach(element => {
                      const indexName = this.variablesService.contacts.findIndex(
                        contact => contact.name === element.name
                      );
                      const indexAddress = this.variablesService.contacts.findIndex(
                        contact => contact.address === element.address
                      );
                      if (indexAddress === -1 && indexName === -1) {
                        this.variablesService.contacts.push(element);
                      }
                      if (indexName !== -1 && indexAddress === -1) {
                        this.variablesService.contacts.push({
                          name: `${element.name} ${this.translate.instant(
                            'CONTACTS.COPY'
                          )}`,
                          address: element.address,
                          notes: element.notes
                        });
                      }
                    });
                  }
                  this.backend.getContactAlias();
<<<<<<< HEAD
                  this.ngZone.run(() => {
                    this.router.navigate(['/contacts']);
                  });
=======
                  this.modalService.prepareModal(
                    'success',
                    'CONTACTS.SUCCESS_IMPORT'
                  );
>>>>>>> contact service
                }
                if (elements.errors.length) {
                  this.modalService.prepareModal(
                    'error',
                    'CONTACTS.ERROR_IMPORT'
                  );
                  console.log(elements.errors);
                }
              }
            });
          } else {
            this.modalService.prepareModal('error', 'CONTACTS.ERROR_TYPE_FILE');
          }
        }
      }
    );
  }

  export() {
    const contacts: Array<Contact> = [];
    this.variablesService.contacts.forEach(contact => {
      delete contact.alias;
      contacts.push(contact);
    });

    this.backend.saveFileDialog(
      '',
      '*',
      this.variablesService.settings.default_path,
      (file_status, file_data) => {
<<<<<<< HEAD
<<<<<<< HEAD
        if (!this.variablesService.contacts.length && !(file_data.error_code === 'CANCELED')) {
          this.modalService.prepareModal('error', 'CONTACTS.ERROR_EMPTY_LIST');
        }
        const path = this.isValid(file_data.path) ? file_data.path : `${file_data.path}.csv`;
        if (file_status && this.isValid(path) && this.variablesService.contacts.length) {
          this.backend.storeFile(path, this.papa.unparse(contacts));
        }
        if (!(file_data.error_code === 'CANCELED') && !this.isValid(path)) {
          this.modalService.prepareModal('error', 'CONTACTS.ERROR_EXPORT');
=======
        if (file_status) {
          this.backend.storeFile(file_data.path, this.papa.unparse(contacts));
>>>>>>> contact service
=======
        if (file_status && this.isValid(file_data.path)) {
          this.backend.storeFile(file_data.path, this.papa.unparse(contacts));
          this.modalService.prepareModal(
            'success',
            'CONTACTS.SUCCESS_EXPORT'
          );
        }
        if (!(file_data.error_code === 'CANCELED') && !this.isValid(file_data.path)) {
          this.modalService.prepareModal('error', 'CONTACTS.ERROR_EXPORT');
>>>>>>> fix export import
        }
      }
    );
  }

  isValid(file) {
    return file.endsWith('.csv');
  }

  back() {
    this.location.back();
  }
}
